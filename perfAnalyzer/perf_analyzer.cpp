// perf_analyzer.cpp
//
// 分析 perf 采集数据，自动给出与人工分析相同思路的结论：
//   1. 数据质量检查（样本数、未解析符号、libc 版本不匹配、栈底异常帧、回溯断裂）
//   2. 整体概况（样本数、CPU 时间、平均 CPU 占用、内核/系统调用占比）
//   3. 线程分布及各线程主要调用路径
//   4. 自身耗时 / 累计耗时排行
//   5. 规则检测：gettid、取时间、select/poll 轮询、64 位软件除法、
//      qsort 比较函数、字符串键 std::map、内存分配、锁、JSON 解析
//   6. 按占比给出优化优先级
//
// 输入（自动识别）：
//   - perf script 的文本输出：perf script -i perf.data --symfs=... --kallsyms=... > out.perf
//   - stackcollapse-perf.pl 生成的折叠栈：comm;f1;f2;...;leaf count
//
// 编译：g++ -O2 -std=c++17 -o perf_analyzer perf_analyzer.cpp
// 用法：./perf_analyzer out.perf --duration 60
//       ./perf_analyzer out.folded --duration 60 --top 20
//       ./perf_analyzer out.perf --folded-out out.folded   (顺便生成折叠栈，供 flamegraph.pl 使用)

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// ---------------------------------------------------------------------------
// 选项
// ---------------------------------------------------------------------------
struct Options
{
	string input;
	string format = "auto"; // auto | script | folded
	double duration = 0;	// 采集时长（秒），用于计算平均 CPU 占用
	double freq = 0;		// 采样频率（Hz），没有 period 信息时用于估算 CPU 时间
	int top = 15;			// 排行榜条数
	double minPct = 1.0;	// 规则检测的最小占比阈值（%）
	string foldedOut;		// 输出折叠栈文件
	bool keepRootAnomalies = false;
	bool markKernel = false; // 输出折叠栈时给内核帧加 _[k]
	string comm;			 // 只分析某个线程名
};

static void usage(const char* prog)
{
	cerr << "用法: " << prog << " <perf script 输出或折叠栈文件> [选项]\n"
		 << "  --format auto|script|folded  输入格式（默认自动识别）\n"
		 << "  --duration <秒>              采集时长，用于计算平均 CPU 占用\n"
		 << "  --freq <Hz>                  采样频率（没有 period 时用于估算 CPU 时间）\n"
		 << "  --top <N>                    排行榜条数（默认 15）\n"
		 << "  --min-pct <百分比>           规则检测阈值（默认 1.0）\n"
		 << "  --comm <线程名>              只分析指定线程\n"
		 << "  --keep-root-anomalies        保留栈底的 [perf-N.map]/[JIT]/[unknown] 帧\n"
		 << "  --folded-out <文件>          输出折叠栈（可直接交给 flamegraph.pl）\n"
		 << "  --mark-kernel                折叠栈中内核帧加 _[k] 后缀\n";
}

// ---------------------------------------------------------------------------
// 字符串工具
// ---------------------------------------------------------------------------
static inline string trim(const string& s)
{
	size_t b = s.find_first_not_of(" \t\r\n");
	if (b == string::npos)
		return "";
	size_t e = s.find_last_not_of(" \t\r\n");
	return s.substr(b, e - b + 1);
}

static inline bool startsWith(const string& s, const string& p)
{
	return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

static inline bool endsWith(const string& s, const string& p)
{
	return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

static inline bool contains(const string& s, const string& p) { return s.find(p) != string::npos; }

static inline string toLower(string s)
{
	for (auto& c : s)
		c = (char)tolower((unsigned char)c);
	return s;
}

static vector<string> splitWs(const string& s)
{
	vector<string> out;
	istringstream is(s);
	string t;
	while (is >> t)
		out.push_back(t);
	return out;
}

static string baseName(const string& path)
{
	size_t p = path.find_last_of('/');
	return p == string::npos ? path : path.substr(p + 1);
}

// 去掉 glibc 版本后缀：sem_trywait@@GLIBC_2.4 -> sem_trywait
static string stripVersion(const string& s)
{
	size_t p = s.find('@');
	if (p != string::npos && p > 0)
		return s.substr(0, p);
	return s;
}

// 显示用：缩短 C++ 长类型名
static string pretty(const string& name, size_t maxLen = 110)
{
	string s = name;
	const string longStr = "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >";
	size_t p;
	while ((p = s.find(longStr)) != string::npos)
		s.replace(p, longStr.size(), "std::string");
	while ((p = s.find("std::string const")) != string::npos)
		s.replace(p, 17, "const std::string");
	if (s.size() > maxLen)
	{
		// 折叠第二层以上的模板参数
		string r;
		int depth = 0;
		for (char c : s)
		{
			if (c == '<')
			{
				if (depth == 0)
					r += '<';
				else if (depth == 1)
					r += "...";
				depth++;
			}
			else if (c == '>')
			{
				depth--;
				if (depth == 0)
					r += '>';
			}
			else if (depth <= 1)
			{
				r += c;
			}
		}
		s = r;
	}
	if (s.size() > maxLen)
		s = s.substr(0, maxLen - 3) + "...";
	return s;
}

static string pctStr(double v, double total)
{
	ostringstream os;
	os << fixed << setprecision(1) << (total > 0 ? 100.0 * v / total : 0.0) << "%";
	return os.str();
}

static string numStr(double v, int prec = 0)
{
	ostringstream os;
	os << fixed << setprecision(prec) << v;
	return os.str();
}

// ---------------------------------------------------------------------------
// 函数分类
// ---------------------------------------------------------------------------
static const unordered_set<string> kLibcNames = {
	"malloc",
	"free",
	"cfree",
	"realloc",
	"calloc",
	"memalign",
	"posix_memalign",
	"_int_malloc",
	"_int_free",
	"_int_realloc",
	"_int_memalign",
	"malloc_consolidate",
	"memcpy",
	"memmove",
	"memset",
	"memcmp",
	"memchr",
	"strlen",
	"strnlen",
	"strcmp",
	"strncmp",
	"strcasecmp",
	"strncasecmp",
	"strcpy",
	"strncpy",
	"strcat",
	"strchr",
	"strrchr",
	"strstr",
	"strtol",
	"strtoul",
	"strtod",
	"atoi",
	"atof",
	"qsort",
	"qsort_r",
	"bsearch",
	"printf",
	"fprintf",
	"sprintf",
	"snprintf",
	"vsnprintf",
	"vfprintf",
	"vsprintf",
	"fwrite",
	"fread",
	"fflush",
	"fputs",
	"puts",
	"fopen",
	"fclose",
	"fgets",
	"gettid",
	"getpid",
	"select",
	"pselect",
	"poll",
	"ppoll",
	"epoll_wait",
	"epoll_pwait",
	"nanosleep",
	"clock_nanosleep",
	"usleep",
	"sleep",
	"sched_yield",
	"clock_gettime",
	"gettimeofday",
	"time",
	"read",
	"write",
	"pread",
	"pwrite",
	"readv",
	"writev",
	"ioctl",
	"fcntl",
	"open",
	"open64",
	"close",
	"lseek",
	"stat",
	"fstat",
	"lstat",
	"stat64",
	"fstat64",
	"recv",
	"recvfrom",
	"recvmsg",
	"send",
	"sendto",
	"sendmsg",
	"accept",
	"connect",
	"socket",
	"bind",
	"listen",
	"sem_wait",
	"sem_trywait",
	"sem_timedwait",
	"sem_post",
	"pthread_mutex_lock",
	"pthread_mutex_unlock",
	"pthread_mutex_trylock",
	"pthread_cond_wait",
	"pthread_cond_timedwait",
	"pthread_cond_signal",
	"pthread_cond_broadcast",
	"pthread_create",
	"pthread_join",
	"start_thread",
	"clone",
	"thread_start",
	"syscall",
	"abort",
	"raise",
};

// 系统调用入口（用于判断一个样本是否处于系统调用中）
static bool isSyscallEntry(const string& n)
{
	return startsWith(n, "el0_svc") || n == "vector_swi" || n == "ret_fast_syscall" || startsWith(n, "entry_SYSCALL") ||
		   n == "do_syscall_64" || startsWith(n, "__arm64_sys_") || startsWith(n, "__arm64_compat_sys_") ||
		   startsWith(n, "__x64_sys_") || startsWith(n, "__ia32_sys_");
}

// 内核函数（折叠栈没有 _[k] 标记时的启发式判断）
static bool looksLikeKernel(const string& n)
{
	static const vector<string> prefixes = {
		"el0_",
		"el1_",
		"__arm64_",
		"do_",
		"sys_",
		"__sys_",
		"ksys_",
		"__se_sys_",
		"compat_",
		"__compat_",
		"schedule",
		"__schedule",
		"_raw_spin",
		"_raw_read",
		"_raw_write",
		"finish_task_switch",
		"hrtimer_",
		"ktime_",
		"futex_",
		"__fget",
		"__fdget",
		"fget",
		"fpsimd_",
		"vector_swi",
		"ret_fast_syscall",
		"entry_SYSCALL",
		"irq_",
		"gic_",
		"__handle_",
		"handle_",
		"__do_softirq",
		"tick_",
		"__task_pid_nr_ns",
		"copy_to_user",
		"copy_from_user",
		"__copy_",
		"__arch_copy",
		"vfs_",
		"path_",
		"filename_lookup",
		"user_path",
		"lookup_",
		"__d_lookup",
		"d_alloc",
		"kmem_cache",
		"__kmalloc",
		"kfree",
		"__wake_up",
		"try_to_wake_up",
		"wake_up",
		"pick_next_task",
		"update_curr",
		"enqueue_",
		"dequeue_",
		"account_",
		"rcu_",
		"__rcu",
		"mutex_",
		"__mutex",
		"down_",
		"up_read",
		"up_write",
		"page_fault",
		"do_page_fault",
		"__do_page_fault",
		"handle_mm_fault",
		"__handle_mm_fault",
		"el0_da",
		"el0_ia",
		"sock_",
		"tcp_",
		"udp_",
		"ip_",
		"__sock",
		"inet_",
		"poll_",
		"pollwake",
		"__pollwait",
		"unix_",
		"pipe_",
		"eventfd_",
		"ep_",
		"timerqueue_",
		"clockevents_",
		"arch_timer",
		"cpuidle",
		"__softirqentry",
		"ttwu_",
		"check_preempt",
		"resched_",
		"native_",
		"posix_",
		"common_nsleep",
		"nanosleep_",
		"seq_",
		"proc_",
		"kernfs_",
		"__audit",
		"audit_",
		"security_",
		"selinux_",
		"apparmor_",
		"refcount_",
		"kref_",
		"fput",
		"fd_install",
		"__close_fd",
		"ksize",
		"memcg_",
		"mod_timer",
		"add_timer",
	};
	for (const auto& p : prefixes)
		if (startsWith(n, p))
			return true;
	return false;
}

// 栈底异常帧：回溯越过栈底时落到匿名可执行内存（[perf-PID.map]、[JIT]）或未知地址
static bool isRootAnomaly(const string& n)
{
	return startsWith(n, "[perf-") || startsWith(n, "[JIT") || n == "[unknown]";
}

// 未解析帧：只有库名或 [unknown]
static bool isUnresolved(const string& n) { return (!n.empty() && n[0] == '[') || startsWith(n, "0x"); }

// 库函数 / 运行时函数（非业务代码）
static bool isLibName(const string& raw)
{
	string n = stripVersion(raw);
	if (n.empty())
		return true;
	if (n[0] == '[')
		return true;
	if (startsWith(n, "0x"))
		return true;
	if (startsWith(n, "std::") || startsWith(n, "__gnu_cxx::") || startsWith(n, "__cxxabiv1::") ||
		startsWith(n, "operator new") || startsWith(n, "operator delete") || startsWith(n, "typeinfo") ||
		startsWith(n, "void std::") || startsWith(n, "bool std::") || startsWith(n, "__cxa_"))
		return true;
	if (startsWith(n, "_"))
		return true; // __xxx / _int_malloc / _dbus_xxx 等内部函数
	static const vector<string> libPrefixes = {
		"dbus_",
		"g_",
		"tdb_",
		"sqlite3",
		"lua_",
		"luaL_",
		"luaV_",
		"luaD_",
		"xml",
		"json_object",
		"json_tokener",
		"SSL_",
		"ssl3_",
		"EVP_",
		"CRYPTO_",
		"BIO_",
		"z_",
		"inflate",
		"deflate",
	};
	for (const auto& p : libPrefixes)
		if (startsWith(n, p))
			return true;
	if (kLibcNames.count(n))
		return true;
	return false;
}

// libc 版本不匹配时常见的"不可能出现在热点里"的函数名
static const unordered_set<string> kSuspiciousLibc = {
	"__fgetspent_r",
	"fgetspent_r",
	"__clearenv",
	"clearenv",
	"envz_entry",
	"getprotobynumber_r",
	"__tdelete",
	"tdelete",
	"__argz_replace",
	"argz_replace",
	"fts_close",
	"__strcasestr",
	"tr_reallochook",
	"__malloc_info.part.9",
	"getpwnam_r",
	"getgrnam_r",
	"__nss_lookup",
	"_nl_find_locale",
};

// ---------------------------------------------------------------------------
// 数据模型
// ---------------------------------------------------------------------------
struct Interner
{
	unordered_map<string, int> m;
	vector<string> v;
	int id(const string& s)
	{
		auto it = m.find(s);
		if (it != m.end())
			return it->second;
		int i = (int)v.size();
		v.push_back(s);
		m.emplace(s, i);
		return i;
	}
	const string& str(int i) const { return v[i]; }
};

struct Stack
{
	vector<int> f;	// 从根到叶：f[0] 为线程名，最后一个为采样点所在函数
	vector<char> k; // 对应帧是否为内核帧
	double w = 0;	// 样本数
};

struct Dataset
{
	Interner names;
	vector<Stack> stacks;
	double totalSamples = 0;
	double periodSum = 0; // period 累加（cpu-clock 时单位为 ns）
	string event;		  // 采样事件名
	bool periodIsNs = false;
	double anomalyStripped = 0; // 去掉栈底异常帧的样本数
	string inputFormat;
};

// ---------------------------------------------------------------------------
// 解析 perf script 输出
// ---------------------------------------------------------------------------
static bool isTimestampTok(const string& t)
{
	if (t.size() < 2 || t.back() != ':')
		return false;
	bool dot = false;
	for (size_t i = 0; i + 1 < t.size(); i++)
	{
		if (t[i] == '.')
			dot = true;
		else if (!isdigit((unsigned char)t[i]))
			return false;
	}
	return dot;
}

static bool isPidTok(const string& t)
{
	if (t.empty())
		return false;
	for (char c : t)
		if (!isdigit((unsigned char)c) && c != '/')
			return false;
	return isdigit((unsigned char)t[0]);
}

static bool isNumTok(const string& t)
{
	if (t.empty())
		return false;
	for (char c : t)
		if (!isdigit((unsigned char)c))
			return false;
	return true;
}

struct Frame
{
	string name;
	bool kernel;
};

static Frame parseFrameLine(const string& line)
{
	string t = trim(line);
	size_t sp = t.find_first_of(" \t");
	string rest = sp == string::npos ? "" : trim(t.substr(sp + 1));
	string dso;
	if (!rest.empty() && rest.back() == ')')
	{
		size_t p = rest.rfind(" (");
		if (p != string::npos)
		{
			dso = rest.substr(p + 2, rest.size() - p - 3);
			rest = trim(rest.substr(0, p));
		}
		else if (rest[0] == '(')
		{
			dso = rest.substr(1, rest.size() - 2);
			rest.clear();
		}
	}
	size_t plus = rest.rfind("+0x");
	if (plus != string::npos && plus > 0)
		rest = rest.substr(0, plus);
	rest = stripVersion(rest);

	bool kernel = contains(dso, "kernel.kallsyms") || startsWith(dso, "[kernel") || endsWith(dso, ".ko") ||
				  (startsWith(dso, "[") && endsWith(dso, ".ko]"));
	string name = rest;
	if (name.empty() || name == "[unknown]")
	{
		if (!dso.empty() && dso != "[unknown]")
		{
			string b = baseName(dso);
			if (!b.empty() && b[0] == '[')
				name = b;
			else
				name = "[" + b + "]";
		}
		else
		{
			name = "[unknown]";
		}
	}
	return {name, kernel};
}

static void addStack(Dataset& ds, const Options& opt, const string& comm, vector<Frame>& leafFirst, double w)
{
	if (!opt.comm.empty() && comm != opt.comm)
		return;
	Stack st;
	st.w = w;
	st.f.push_back(ds.names.id(comm.empty() ? "[unknown-comm]" : comm));
	st.k.push_back(0);
	for (auto it = leafFirst.rbegin(); it != leafFirst.rend(); ++it)
	{
		st.f.push_back(ds.names.id(it->name));
		st.k.push_back(it->kernel ? 1 : 0);
	}
	if (!opt.keepRootAnomalies)
	{
		bool stripped = false;
		while (st.f.size() > 2 && isRootAnomaly(ds.names.str(st.f[1])))
		{
			st.f.erase(st.f.begin() + 1);
			st.k.erase(st.k.begin() + 1);
			stripped = true;
		}
		if (stripped)
			ds.anomalyStripped += w;
	}
	ds.totalSamples += w;
	ds.stacks.push_back(std::move(st));
}

static bool parsePerfScript(istream& in, Dataset& ds, const Options& opt)
{
	string line, comm;
	vector<Frame> frames;
	bool inSample = false;
	double period = 0;
	auto flush = [&]()
	{
		if (inSample)
		{
			addStack(ds, opt, comm, frames, 1.0);
			ds.periodSum += period;
		}
		frames.clear();
		inSample = false;
		period = 0;
	};
	while (getline(in, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (trim(line).empty())
		{
			flush();
			continue;
		}
		if (line[0] == '#')
			continue;
		if (line[0] == ' ' || line[0] == '\t')
		{
			if (inSample)
				frames.push_back(parseFrameLine(line));
			continue;
		}
		// 样本头：comm pid[/tid] [cpu] time: [period] event:
		flush();
		vector<string> tok = splitWs(line);
		int ts = -1;
		for (int i = 0; i < (int)tok.size(); i++)
			if (isTimestampTok(tok[i]))
			{
				ts = i;
				break;
			}
		if (ts < 1)
			continue;
		int pidIdx = ts - 1;
		if (pidIdx >= 0 && tok[pidIdx].size() > 2 && tok[pidIdx][0] == '[')
			pidIdx--;
		if (pidIdx < 0 || !isPidTok(tok[pidIdx]))
			continue;
		comm.clear();
		for (int i = 0; i < pidIdx; i++)
		{
			if (i)
				comm += ' ';
			comm += tok[i];
		}
		int ev = ts + 1;
		if (ev < (int)tok.size() && isNumTok(tok[ev]))
		{
			period = stod(tok[ev]);
			ev++;
		}
		if (ev < (int)tok.size() && ds.event.empty())
		{
			string e = tok[ev];
			if (!e.empty() && e.back() == ':')
				e.pop_back();
			ds.event = e;
		}
		inSample = true;
	}
	flush();
	if (contains(ds.event, "cpu-clock") || contains(ds.event, "task-clock"))
		ds.periodIsNs = true;
	ds.inputFormat = "perf script";
	return !ds.stacks.empty();
}

// ---------------------------------------------------------------------------
// 解析折叠栈
// ---------------------------------------------------------------------------
static bool parseFolded(istream& in, Dataset& ds, const Options& opt)
{
	struct Raw
	{
		vector<Frame> leafFirst;
		string comm;
		double cnt;
	};
	vector<Raw> raws;
	string line;
	while (getline(in, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;
		size_t sp = line.find_last_of(' ');
		if (sp == string::npos)
			continue;
		double cnt = 0;
		try
		{
			cnt = stod(line.substr(sp + 1));
		}
		catch (...)
		{
			continue;
		}
		string path = line.substr(0, sp);
		vector<string> parts;
		size_t start = 0;
		while (true)
		{
			size_t p = path.find(';', start);
			parts.push_back(path.substr(start, p == string::npos ? string::npos : p - start));
			if (p == string::npos)
				break;
			start = p + 1;
		}
		if (parts.empty())
			continue;
		Raw r;
		r.comm = parts[0];
		r.cnt = cnt;
		for (size_t i = parts.size(); i-- > 1;)
		{
			string n = parts[i];
			bool k = false;
			if (endsWith(n, "_[k]"))
			{
				n = n.substr(0, n.size() - 4);
				k = true;
			}
			else if (endsWith(n, "_[j]") || endsWith(n, "_[i]") || endsWith(n, "_[w]"))
				n = n.substr(0, n.size() - 4);
			else
				k = looksLikeKernel(n);
			r.leafFirst.push_back({stripVersion(n), k});
		}
		raws.push_back(std::move(r));
	}
	if (raws.empty())
		return false;

	// 自动识别计数单位：stackcollapse 新版本会累加 period（cpu-clock 为纳秒）
	double mn = 1e300;
	for (auto& r : raws)
		if (r.cnt > 0)
			mn = min(mn, r.cnt);
	double unit = 1;
	if (mn >= 100000)
	{
		bool allMultiple = true;
		for (auto& r : raws)
		{
			double q = r.cnt / mn;
			if (fabs(q - llround(q)) > 0.01)
			{
				allMultiple = false;
				break;
			}
		}
		if (allMultiple)
			unit = mn;
	}
	for (auto& r : raws)
	{
		addStack(ds, opt, r.comm, r.leafFirst, r.cnt / unit);
		if (unit > 1)
			ds.periodSum += r.cnt;
	}
	if (unit > 1)
	{
		ds.periodIsNs = true;
		ds.event = "cpu-clock（由计数单位推断，每样本 " + numStr(unit) + " ns）";
	}
	ds.inputFormat = "折叠栈";
	return !ds.stacks.empty();
}

// ---------------------------------------------------------------------------
// 分析
// ---------------------------------------------------------------------------
struct Finding
{
	string title;
	double weight = 0; // 涉及样本数
	vector<string> details;
	string advice;
	string effort; // 改动难度
};

class Analyzer
{
  public:
	Analyzer(Dataset& d, const Options& o) : ds(d), opt(o), N(ds.names.v.size()) {}

	void run()
	{
		aggregate();
		printHeader();
		printQuality();
		printOverview();
		printThreads();
		printTopLists();
		detect();
		printFindings();
		printCaveats();
	}

  private:
	Dataset& ds;
	const Options& opt;
	size_t N;
	double W = 0;

	vector<double> selfW, inclW;
	map<string, double> threadW;
	double kernelLeafW = 0, syscallW = 0;
	map<string, double> syscallWrapperW; // 用户态系统调用包装函数 -> 样本
	vector<Finding> findings;

	const string& nm(int id) const { return ds.names.str(id); }
	bool isApp(const Stack& s, size_t i) const { return i > 0 && !s.k[i] && !isLibName(nm(s.f[i])); }

	// ---- 聚合 ----
	void aggregate()
	{
		W = ds.totalSamples;
		selfW.assign(N, 0);
		inclW.assign(N, 0);
		vector<int> stamp(N, -1);
		for (size_t si = 0; si < ds.stacks.size(); si++)
		{
			const Stack& s = ds.stacks[si];
			threadW[nm(s.f[0])] += s.w;
			selfW[s.f.back()] += s.w;
			for (size_t i = 1; i < s.f.size(); i++)
			{
				int id = s.f[i];
				if (stamp[id] != (int)si)
				{
					stamp[id] = (int)si;
					inclW[id] += s.w;
				}
			}
			if (s.k.back())
				kernelLeafW += s.w;
			// 系统调用：找到第一个系统调用入口帧，它上方最近的用户态帧即包装函数
			for (size_t i = 1; i < s.f.size(); i++)
			{
				if (s.k[i] && isSyscallEntry(nm(s.f[i])))
				{
					syscallW += s.w;
					string wrapper = "[未知]";
					for (size_t j = i; j-- > 1;)
					{
						if (!s.k[j] && !isRootAnomaly(nm(s.f[j])))
						{
							wrapper = nm(s.f[j]);
							break;
						}
					}
					syscallWrapperW[wrapper] += s.w;
					break;
				}
			}
		}
	}

	// 包含任一目标函数的样本数（每个样本只计一次）
	double inclOf(const set<string>& targets, const string& thread = "") const
	{
		double w = 0;
		for (const auto& s : ds.stacks)
		{
			if (!thread.empty() && nm(s.f[0]) != thread)
				continue;
			for (size_t i = 1; i < s.f.size(); i++)
				if (targets.count(stripVersion(nm(s.f[i]))))
				{
					w += s.w;
					break;
				}
		}
		return w;
	}

	// 目标函数最近的业务调用者
	vector<pair<string, double>> appCallers(const set<string>& targets, size_t n = 5) const
	{
		map<string, double> m;
		for (const auto& s : ds.stacks)
		{
			for (size_t i = s.f.size(); i-- > 1;)
			{
				if (!targets.count(stripVersion(nm(s.f[i]))))
					continue;
				string caller = "（调用者未知，回溯在库或栈底中断）";
				for (size_t j = i; j-- > 1;)
					if (isApp(s, j) && !targets.count(stripVersion(nm(s.f[j]))))
					{
						caller = nm(s.f[j]);
						break;
					}
				m[caller] += s.w;
				break;
			}
		}
		vector<pair<string, double>> v(m.begin(), m.end());
		sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
		if (v.size() > n)
			v.resize(n);
		return v;
	}

	// 各线程中目标函数的占比
	vector<pair<string, double>> byThread(const set<string>& targets) const
	{
		map<string, double> m;
		for (const auto& s : ds.stacks)
			for (size_t i = 1; i < s.f.size(); i++)
				if (targets.count(stripVersion(nm(s.f[i]))))
				{
					m[nm(s.f[0])] += s.w;
					break;
				}
		vector<pair<string, double>> v(m.begin(), m.end());
		sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
		return v;
	}

	// ---- 输出 ----
	void printHeader() const
	{
		cout << "# perf 采样数据分析报告\n\n";
		cout << "- 输入文件：" << opt.input << "（" << ds.inputFormat << "）\n";
		if (!ds.event.empty())
			cout << "- 采样事件：" << ds.event << "\n";
		if (!opt.comm.empty())
			cout << "- 只分析线程：" << opt.comm << "\n";
		cout << "\n";
	}

	void printQuality() const
	{
		cout << "## 一、数据质量检查\n\n";
		vector<string> issues;

		if (W < 300)
			issues.push_back("样本数只有 " + numStr(W) +
							 " 个，统计意义很弱，小于 2% 的项基本可视为噪声。建议提高采样频率（-F "
							 "199）、延长采集时间，并在业务繁忙时采集。");
		else if (W < 1000)
			issues.push_back("样本数 " + numStr(W) + " 个，偏少，结论只能作为方向参考，建议采集到几千个样本以上。");

		// 未解析帧
		double unresolvedLeaf = 0;
		map<string, double> unresolvedBy;
		for (const auto& s : ds.stacks)
		{
			const string& leaf = nm(s.f.back());
			if (isUnresolved(leaf) && !s.k.back())
			{
				unresolvedLeaf += s.w;
				unresolvedBy[leaf] += s.w;
			}
		}
		if (W > 0 && unresolvedLeaf / W > 0.05)
		{
			ostringstream os;
			os << "采样点未解析出函数名的比例为 " << pctStr(unresolvedLeaf, W) << "，主要是：";
			vector<pair<string, double>> v(unresolvedBy.begin(), unresolvedBy.end());
			sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
			for (size_t i = 0; i < v.size() && i < 4; i++)
				os << (i ? "、" : "") << v[i].first << "（" << pctStr(v[i].second, W) << "）";
			os << "。只显示库名说明 --symfs 中缺少该文件、文件是 strip 过的，或路径与板子上不一致；"
				  "库内部的 static 函数在 strip 过的库里本来就无法还原。";
			issues.push_back(os.str());
		}

		// libc 版本不匹配
		double suspicious = 0;
		set<string> hit;
		for (int id = 0; id < (int)N; id++)
		{
			string n = stripVersion(nm(id));
			if (kSuspiciousLibc.count(n) && inclW[id] > 0)
			{
				suspicious += inclW[id];
				hit.insert(n);
			}
		}
		if (W > 0 && suspicious / W > 0.01)
		{
			string list;
			for (auto& h : hit)
				list += (list.empty() ? "" : "、") + h;
			issues.push_back("出现了 " + list + " 等通常不可能成为热点的 libc 函数（合计 " + pctStr(suspicious, W) +
							 "），很可能是 symfs 中的 libc 与板子上的版本不一致，函数名被\"对错号\"。"
							 "请直接使用从板子上拷贝的 libc，或核对两者的 Build ID。");
		}

		// 栈底异常帧
		if (W > 0 && ds.anomalyStripped / W > 0.3)
			issues.push_back(pctStr(ds.anomalyStripped, W) + " 的调用栈底部是 [perf-N.map]/[JIT]/[unknown] 帧，"
															 "这是帧指针回溯越过栈底、落到可执行匿名内存（如可执行线程"
															 "栈）造成的，已在分析中去除，不影响上层热点判断。");

		// 回溯断裂：用户态只有一层
		double shallow = 0;
		for (const auto& s : ds.stacks)
		{
			int userFrames = 0;
			for (size_t i = 1; i < s.f.size(); i++)
				if (!s.k[i])
					userFrames++;
			if (userFrames <= 1)
				shallow += s.w;
		}
		if (W > 0 && shallow / W > 0.25)
			issues.push_back(pctStr(shallow, W) +
							 " 的样本用户态只有一层栈帧，调用者丢失。常见原因是采样点位于没有帧指针的库函数中"
							 "（如 libc 的 qsort 回调、memcpy），或程序未用 -marm -mapcs-frame（ARM32）/ "
							 "-fno-omit-frame-pointer 编译。");

		// 不合理调用链无法自动判定，给出提示
		issues.push_back("工具无法自动判断业务调用链是否合理：如果看到明显不符合业务逻辑的调用关系，"
						 "说明解析用的程序与板子上运行的版本不一致，请用 md5sum 核对后再分析。");

		for (auto& s : issues)
			cout << "- " << s << "\n";
		cout << "\n";
	}

	void printOverview() const
	{
		cout << "## 二、整体概况\n\n";
		cout << "| 指标 | 数值 |\n| --- | --- |\n";
		cout << "| 样本数 | " << numStr(W) << " |\n";
		double cpuSec = -1;
		if (ds.periodIsNs && ds.periodSum > 0)
			cpuSec = ds.periodSum / 1e9;
		else if (opt.freq > 0)
			cpuSec = W / opt.freq;
		if (cpuSec >= 0)
			cout << "| CPU 时间 | 约 " << numStr(cpuSec, 2) << " 秒 |\n";
		if (cpuSec >= 0 && opt.duration > 0)
			cout << "| 平均 CPU 占用 | 单核约 " << numStr(100.0 * cpuSec / opt.duration, 1) << "%（采集 "
				 << numStr(opt.duration) << " 秒） |\n";
		cout << "| 采样点在内核中 | " << pctStr(kernelLeafW, W) << " |\n";
		cout << "| 处于系统调用中 | " << pctStr(syscallW, W) << " |\n";
		cout << "| 线程数 | " << threadW.size() << " |\n\n";

		if (cpuSec >= 0 && opt.duration > 0 && cpuSec / opt.duration < 0.2)
			cout << "进程整体 CPU 占用较低，下面的热点是相对比例，优化后对整体 CPU 的绝对收益有限；"
					"如果要排查高负载或卡顿，应在问题出现时采集。\n\n";
		if (cpuSec >= 0 && opt.duration <= 0)
			cout << "提示：加上 --duration <采集秒数> 可以计算平均 CPU 占用。\n\n";

		if (!syscallWrapperW.empty())
		{
			vector<pair<string, double>> v(syscallWrapperW.begin(), syscallWrapperW.end());
			sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
			cout << "系统调用按用户态函数分布：\n\n| 函数 | 占全部样本 |\n| --- | --- |\n";
			for (size_t i = 0; i < v.size() && i < 8; i++)
				cout << "| " << pretty(v[i].first) << " | " << pctStr(v[i].second, W) << " |\n";
			cout << "\n";
		}
	}

	void printThreads() const
	{
		cout << "## 三、线程分布\n\n";
		vector<pair<string, double>> v(threadW.begin(), threadW.end());
		sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
		cout << "| 线程 | 占比 | 主要业务函数（累计占全部样本） |\n| --- | --- | --- |\n";
		for (auto& t : v)
		{
			if (t.second / W * 100 < 0.5)
				continue;
			// 该线程内累计占比最高的几个业务函数
			map<int, double> inc;
			for (const auto& s : ds.stacks)
			{
				if (nm(s.f[0]) != t.first)
					continue;
				set<int> seen;
				for (size_t i = 1; i < s.f.size(); i++)
					if (isApp(s, i) && seen.insert(s.f[i]).second)
						inc[s.f[i]] += s.w;
			}
			vector<pair<int, double>> iv(inc.begin(), inc.end());
			sort(iv.begin(), iv.end(), [](auto& a, auto& b) { return a.second > b.second; });
			string tops;
			int shown = 0;
			for (auto& p : iv)
			{
				if (shown >= 3)
					break;
				const string& n = nm(p.first);
				if (n == "PosixTaskWrapper" || n == "PosixTaskSpawn")
					continue;
				tops += (tops.empty() ? "" : "；") + pretty(n, 60) + " " + pctStr(p.second, W);
				shown++;
			}
			cout << "| " << t.first << " | " << pctStr(t.second, W) << " | " << (tops.empty() ? "—" : tops) << " |\n";
		}
		cout << "\n";

		// 主要线程的最热调用路径
		cout << "主要线程的最热调用路径（业务函数，从上到下为调用关系）：\n\n";
		int shownThreads = 0;
		for (auto& t : v)
		{
			if (shownThreads >= 3 || t.second / W < 0.08)
				break;
			printHotPath(t.first, t.second);
			shownThreads++;
		}
	}

	// 沿累计占比最大的子节点向下走，打印线程内的主干调用路径
	void printHotPath(const string& thread, double threadWeight) const
	{
		struct Node
		{
			double w = 0;
			map<int, Node> ch;
		};
		Node root;
		for (const auto& s : ds.stacks)
		{
			if (nm(s.f[0]) != thread)
				continue;
			Node* cur = &root;
			cur->w += s.w;
			for (size_t i = 1; i < s.f.size(); i++)
			{
				if (!isApp(s, i))
					continue;
				cur = &cur->ch[s.f[i]];
				cur->w += s.w;
			}
		}
		cout << "**" << thread << "**（" << pctStr(threadWeight, W) << "）\n\n```\n";
		const Node* cur = &root;
		int depth = 0;
		while (!cur->ch.empty() && depth < 10)
		{
			const pair<const int, Node>* best = nullptr;
			for (auto& c : cur->ch)
				if (!best || c.second.w > best->second.w)
					best = &c;
			if (best->second.w / W < 0.02)
				break;
			cout << string(depth * 2, ' ') << pretty(nm(best->first), 90) << "  " << pctStr(best->second.w, W) << "\n";
			cur = &best->second;
			depth++;
		}
		cout << "```\n\n";
	}

	void printTopLists() const
	{
		cout << "## 四、热点排行\n\n";
		vector<int> ids(N);
		for (size_t i = 0; i < N; i++)
			ids[i] = (int)i;

		cout << "### 自身耗时（采样点直接落在该函数）\n\n| 排名 | 函数 | 占比 | 类型 |\n| --- | --- | --- | --- |\n";
		sort(ids.begin(), ids.end(), [&](int a, int b) { return selfW[a] > selfW[b]; });
		for (int r = 0, i = 0; i < (int)ids.size() && r < opt.top; i++)
		{
			if (selfW[ids[i]] <= 0)
				break;
			cout << "| " << ++r << " | " << pretty(nm(ids[i])) << " | " << pctStr(selfW[ids[i]], W) << " | "
				 << kindOf(ids[i]) << " |\n";
		}
		cout << "\n### 累计耗时（包含其调用的函数，只列业务函数）\n\n| 排名 | 函数 | 占比 |\n| --- | --- | --- |\n";
		sort(ids.begin(), ids.end(), [&](int a, int b) { return inclW[a] > inclW[b]; });
		for (int r = 0, i = 0; i < (int)ids.size() && r < opt.top; i++)
		{
			const string& n = nm(ids[i]);
			if (inclW[ids[i]] <= 0)
				break;
			if (isLibName(n) || threadW.count(n) || isKernelId(ids[i]))
				continue;
			if (n == "PosixTaskWrapper" || n == "PosixTaskSpawn")
				continue;
			cout << "| " << ++r << " | " << pretty(n) << " | " << pctStr(inclW[ids[i]], W) << " |\n";
		}
		cout << "\n";
	}

	bool isKernelId(int id) const
	{
		for (const auto& s : ds.stacks)
			for (size_t i = 1; i < s.f.size(); i++)
				if (s.f[i] == id)
					return s.k[i];
		return false;
	}

	string kindOf(int id) const
	{
		const string& n = nm(id);
		if (isKernelId(id))
			return "内核";
		if (isUnresolved(n))
			return "未解析";
		if (isLibName(n))
			return "库/运行时";
		return "业务";
	}

	// ---- 规则检测 ----
	void addCallerDetails(Finding& f, const set<string>& targets) const
	{
		for (auto& c : appCallers(targets))
			f.details.push_back("调用者 " + pretty(c.first, 90) + "：" + pctStr(c.second, W));
		auto bt = byThread(targets);
		if (!bt.empty())
		{
			string s = "所在线程：";
			for (size_t i = 0; i < bt.size() && i < 4; i++)
				s += (i ? "、" : "") + bt[i].first + " " + pctStr(bt[i].second, W);
			f.details.push_back(s);
		}
	}

	bool over(double w) const { return W > 0 && 100.0 * w / W >= opt.minPct; }

	void detect()
	{
		// 1. gettid
		{
			double w = inclOf({"gettid", "__gettid"});
			if (over(w))
			{
				Finding f;
				f.title = "频繁调用 gettid 获取线程号";
				f.weight = w;
				addCallerDetails(f, {"gettid", "__gettid"});
				f.advice = "gettid 每次都是一次系统调用。在线程入口获取一次并缓存（局部变量或 "
						   "thread_local），不要在循环、日志或加锁路径中反复调用。";
				f.effort = "很小";
				findings.push_back(f);
			}
		}
		// 2. 取时间
		{
			set<string> t = {"clock_gettime", "__clock_gettime", "gettimeofday", "__gettimeofday", "time"};
			double w = inclOf(t);
			if (over(w))
			{
				Finding f;
				f.title = "频繁获取系统时间";
				f.weight = w;
				addCallerDetails(f, t);
				double sys = 0;
				for (auto& p : syscallWrapperW)
					if (t.count(stripVersion(p.first)))
						sys += p.second;
				if (sys > 0)
					f.details.push_back("其中进入内核的部分：" + pctStr(sys, W) +
										"（未走 vDSO 快速路径，64 位内核上的 32 位程序常见）");
				f.advice = "一个处理周期内只取一次时间并复用；对精度要求不高的地方可以用缓存的时间戳。";
				f.effort = "小";
				findings.push_back(f);
			}
		}
		// 3. 64 位软件除法
		{
			set<string> t = {"__udivmoddi4", "__aeabi_uldivmod", "__aeabi_ldivmod", "__divdi3", "__udivdi3", "__moddi3",
				"__umoddi3", "__divmoddi4"};
			double w = inclOf(t);
			if (over(w))
			{
				Finding f;
				f.title = "64 位整数软件除法（__udivmoddi4 等）";
				f.weight = w;
				addCallerDetails(f, t);
				f.advice =
					"32 位 ARM 没有 64 位除法指令，int64/uint64 的 / 和 % 由软件实现，开销较大。常见于时间单位换算，"
					"可在一个周期内只换算一次、改用移位或乘法近似，或直接保持统一的时间单位避免除法。";
				f.effort = "小";
				findings.push_back(f);
			}
		}
		// 4. 排序比较函数（qsort 回调等）
		for (int id = 0; id < (int)N; id++)
		{
			const string& n = nm(id);
			string low = toLower(n);
			bool isCmp = (contains(low, "compare") || contains(low, "cmp")) && !startsWith(low, "memcmp") &&
						 !startsWith(low, "strcmp") && !startsWith(low, "strncmp") && !contains(low, "strcasecmp") &&
						 !isLibName(n);
			if (!isCmp || !over(selfW[id]))
				continue;
			// 统计调用者类型：库函数/栈底 -> 回调式排序
			double viaLib = 0, total = 0;
			map<string, double> thr;
			for (const auto& s : ds.stacks)
			{
				for (size_t i = s.f.size(); i-- > 1;)
				{
					if (s.f[i] != id)
						continue;
					total += s.w;
					thr[nm(s.f[0])] += s.w;
					if (i == 1 || isLibName(nm(s.f[i - 1])) || isRootAnomaly(nm(s.f[i - 1])))
						viaLib += s.w;
					break;
				}
			}
			string mainThread;
			double mt = 0;
			for (auto& p : thr)
				if (p.second > mt)
				{
					mt = p.second;
					mainThread = p.first;
				}
			// 同线程中 libc 内部未解析函数的样本（排序算法本体）
			double libInternal = 0;
			for (const auto& s : ds.stacks)
				if (nm(s.f[0]) == mainThread && startsWith(nm(s.f.back()), "[libc"))
					libInternal += s.w;
			Finding f;
			f.title = "比较函数 " + pretty(n, 70) + " 被频繁调用（疑似反复排序）";
			f.weight = inclW[id] + libInternal;
			f.details.push_back("比较函数自身：" + pctStr(selfW[id], W));
			if (total > 0)
				f.details.push_back("其中由库函数或断裂栈调用的比例：" + pctStr(viaLib, total).substr(0) +
									"（qsort 等库函数回调的典型特征）");
			if (!mainThread.empty())
			{
				double tw = threadW.count(mainThread) ? threadW.at(mainThread) : 0;
				f.details.push_back("主要在线程 " + mainThread + "（该线程共 " + pctStr(tw, W) +
									"）；同线程 libc 内部函数 " + pctStr(libInternal, W) +
									"，比较函数 + 排序本体约占该线程的 " +
									(tw > 0 ? pctStr(selfW[id] + libInternal, tw) : string("?")));
			}
			f.advice = "如果每个周期都对整个列表调用 qsort/std::sort，改用最小堆（std::priority_queue）或有序插入，"
					   "只处理新增和到期的元素；同时检查比较函数本身是否有不必要的开销。";
			f.effort = "中";
			findings.push_back(f);
		}
		// 5. 字符串为键的 map/unordered_map
		{
			double w = 0;
			set<string> rbNames;
			for (int id = 0; id < (int)N; id++)
			{
				const string& n = nm(id);
				bool rb = (contains(n, "_Rb_tree<std::__cxx11::basic_string") || contains(n, "_Rb_tree<std::string") ||
						   contains(n, "map<std::__cxx11::basic_string") ||
						   contains(n, "_Hashtable<std::__cxx11::basic_string"));
				if (rb)
					rbNames.insert(n);
			}
			if (!rbNames.empty())
				w = inclOf(rbNames);
			set<string> strOps = {"memcmp", "strcmp", "strlen", "strncmp"};
			double strW = 0;
			for (int id = 0; id < (int)N; id++)
			{
				const string& n = stripVersion(nm(id));
				if (strOps.count(n) ||
					(startsWith(n, "std::__cxx11::basic_string") &&
						(contains(n, "_M_construct") || contains(n, "_M_replace") || contains(n, "_M_assign") ||
							contains(n, "compare") || contains(n, "basic_string("))) ||
					contains(n, "basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct"))
					strW += selfW[id];
			}
			if (over(w) || (over(strW) && !rbNames.empty()))
			{
				Finding f;
				f.title = "以字符串为键的 std::map 查找和字符串操作";
				f.weight = w + strW;
				f.details.push_back("红黑树/哈希表（字符串键）相关：" + pctStr(w, W));
				f.details.push_back(
					"字符串比较/长度/构造/赋值（memcmp、strlen、std::string 等）自身：" + pctStr(strW, W));
				for (auto& c : appCallers(rbNames))
					f.details.push_back("查找发起者 " + pretty(c.first, 90) + "：" + pctStr(c.second, W));
				f.advice = "热点路径上按名字字符串查找属性/"
						   "配置。可在初始化时把名字解析成索引、枚举或指针缓存，周期内直接访问；"
						   "避免为查找临时构造 std::string（可用 std::string_view 或透明比较器 std::less<>）。";
				f.effort = "中";
				findings.push_back(f);
			}
		}
		// 6. select/poll/epoll/nanosleep 轮询
		{
			set<string> t = {"select", "__select", "pselect", "poll", "__poll", "ppoll", "epoll_wait", "epoll_pwait",
				"nanosleep", "__nanosleep", "clock_nanosleep", "usleep", "sleep"};
			double w = inclOf(t);
			if (over(w))
			{
				Finding f;
				f.title = "select/poll/sleep 等待类调用占用 CPU（疑似短超时轮询）";
				f.weight = w;
				for (auto& bt : byThread(t))
				{
					double tw = threadW.count(bt.first) ? threadW.at(bt.first) : 0;
					if (bt.second / W * 100 < opt.minPct)
						continue;
					f.details.push_back("线程 " + bt.first + "：" + pctStr(bt.second, W) + "（占该线程 " +
										(tw > 0 ? pctStr(bt.second, tw) : string("?")) + "）");
				}
				int dsId = -1;
				auto it = ds.names.m.find("do_select");
				if (it != ds.names.m.end())
					dsId = it->second;
				if (dsId >= 0 && selfW[dsId] > 0)
					f.details.push_back("内核 do_select 自身：" + pctStr(selfW[dsId], W));
				f.advice = "阻塞等待本身不耗 CPU，占比明显说明线程被频繁唤醒。检查 select/poll 的超时参数是否过短、"
						   "是否在循环中配合 sleep 轮询，尽量改为较长超时或纯事件驱动。";
				f.effort = "小";
				findings.push_back(f);
			}
		}
		// 7. 内存分配
		{
			set<string> t = {"malloc", "free", "cfree", "realloc", "calloc", "_int_malloc", "_int_free", "_int_realloc",
				"_int_memalign", "malloc_consolidate", "operator new(unsigned int)", "operator new(unsigned long)",
				"operator delete(void*)", "operator new", "operator delete", "memalign"};
			double w = inclOf(t);
			if (over(w * 0.5) && over(w))
			{
				Finding f;
				f.title = "内存分配/释放较频繁";
				f.weight = w;
				addCallerDetails(f, t);
				f.advice =
					"检查热点循环中的临时对象、std::string/std::vector 拷贝和 Json 对象构造；可通过 reserve、对象复用、"
					"传引用或 move 减少分配。";
				f.effort = "中";
				findings.push_back(f);
			}
		}
		// 8. 锁/同步
		{
			set<string> t = {"sem_wait", "sem_trywait", "sem_timedwait", "sem_post", "pthread_mutex_lock",
				"pthread_mutex_unlock", "__pthread_mutex_lock", "__pthread_mutex_unlock_usercnt", "pthread_cond_wait",
				"pthread_cond_timedwait", "__lll_lock_wait", "PosixSemTake", "PosixSemGive"};
			double w = inclOf(t);
			if (over(w))
			{
				Finding f;
				f.title = "锁和信号量操作";
				f.weight = w;
				addCallerDetails(f, t);
				f.advice =
					"sem_trywait "
					"等非阻塞操作如果在循环中反复尝试，相当于忙等。检查是否存在轮询式加锁，以及锁的粒度和持有时间。";
				f.effort = "中";
				findings.push_back(f);
			}
		}
		// 9. JSON 解析
		{
			set<string> t;
			for (int id = 0; id < (int)N; id++)
			{
				const string& n = nm(id);
				if (startsWith(n, "Json::Reader::") || startsWith(n, "nlohmann::") || contains(n, "json_parse") ||
					startsWith(n, "json_tokener_"))
					t.insert(n);
			}
			double w = t.empty() ? 0 : inclOf(t);
			if (over(w))
			{
				Finding f;
				f.title = "JSON 解析";
				f.weight = w;
				addCallerDetails(f, t);
				f.advice = "检查是否在周期性路径中重复解析相同的 JSON 字符串，可解析一次后缓存结果。";
				f.effort = "中";
				findings.push_back(f);
			}
		}
		// 10. 系统调用整体占比
		if (W > 0 && syscallW / W > 0.2)
		{
			Finding f;
			f.title = "系统调用整体占比偏高";
			f.weight = syscallW;
			vector<pair<string, double>> v(syscallWrapperW.begin(), syscallWrapperW.end());
			sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
			for (size_t i = 0; i < v.size() && i < 5; i++)
				f.details.push_back(pretty(v[i].first) + "：" + pctStr(v[i].second, W));
			f.advice =
				"进入内核本身有固定开销，处理掉上面占比最高的几类调用（gettid、轮询、取时间）后，这个比例会明显下降。";
			f.effort = "—";
			findings.push_back(f);
		}

		sort(findings.begin(), findings.end(),
			[](const Finding& a, const Finding& b)
			{
				// "系统调用整体占比"是汇总项，放到最后
				bool as = startsWith(a.title, "系统调用整体"), bs = startsWith(b.title, "系统调用整体");
				if (as != bs)
					return bs;
				return a.weight > b.weight;
			});
	}

	void printFindings() const
	{
		cout << "## 五、规则检测结果\n\n";
		if (findings.empty())
		{
			cout << "没有命中任何规则（阈值 " << opt.minPct << "%）。\n\n";
			return;
		}
		int i = 0;
		for (const auto& f : findings)
		{
			cout << "### " << ++i << ". " << f.title << "（约 " << pctStr(f.weight, W) << "）\n\n";
			for (const auto& d : f.details)
				cout << "- " << d << "\n";
			cout << "\n建议：" << f.advice << "\n\n";
		}

		cout << "## 六、优化优先级\n\n| 优先级 | 问题 | 占比 | 改动难度 |\n| --- | --- | --- | --- |\n";
		// 按"占比 / 难度"排序：难度越小越优先
		auto cost = [](const string& e) { return e == "很小" ? 1.0 : e == "小" ? 1.5 : e == "中" ? 2.5 : 4.0; };
		vector<const Finding*> v;
		for (auto& f : findings)
			if (f.effort != "—")
				v.push_back(&f);
		sort(v.begin(), v.end(),
			[&](const Finding* a, const Finding* b)
			{ return a->weight / cost(a->effort) > b->weight / cost(b->effort); });
		int r = 0;
		for (auto* f : v)
			cout << "| " << ++r << " | " << f->title << " | " << pctStr(f->weight, W) << " | " << f->effort << " |\n";
		cout << "\n";
	}

	void printCaveats() const
	{
		cout << "## 七、说明\n\n";
		cout << "- 占比均相对于全部样本；不同规则之间可能有重叠（例如 gettid 同时计入系统调用整体占比）。\n";
		cout << "- CPU 火焰图只反映占用 CPU 的时间，线程阻塞、等待锁、等待 IO 的时间不会出现在这里。\n";
		if (ds.inputFormat == "折叠栈")
			cout << "- 折叠栈输入中内核函数是按名字启发式识别的；使用 perf script "
					"输出作为输入可以得到准确的内核/用户态区分。\n";
		cout << "\n";
	}
};

// ---------------------------------------------------------------------------
// 输出折叠栈
// ---------------------------------------------------------------------------
static void writeFolded(const Dataset& ds, const Options& opt)
{
	map<string, double> agg;
	for (const auto& s : ds.stacks)
	{
		string line;
		for (size_t i = 0; i < s.f.size(); i++)
		{
			if (i)
				line += ';';
			line += ds.names.str(s.f[i]);
			if (opt.markKernel && s.k[i])
				line += "_[k]";
		}
		agg[line] += s.w;
	}
	ofstream out(opt.foldedOut);
	for (auto& p : agg)
		out << p.first << ' ' << llround(p.second) << '\n';
	cerr << "已输出折叠栈：" << opt.foldedOut << "（" << agg.size() << " 条）\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
	Options opt;
	for (int i = 1; i < argc; i++)
	{
		string a = argv[i];
		auto next = [&]() -> string
		{
			if (i + 1 >= argc)
			{
				usage(argv[0]);
				exit(1);
			}
			return argv[++i];
		};
		if (a == "--format")
			opt.format = next();
		else if (a == "--duration")
			opt.duration = stod(next());
		else if (a == "--freq")
			opt.freq = stod(next());
		else if (a == "--top")
			opt.top = stoi(next());
		else if (a == "--min-pct")
			opt.minPct = stod(next());
		else if (a == "--comm")
			opt.comm = next();
		else if (a == "--folded-out")
			opt.foldedOut = next();
		else if (a == "--keep-root-anomalies")
			opt.keepRootAnomalies = true;
		else if (a == "--mark-kernel")
			opt.markKernel = true;
		else if (a == "-h" || a == "--help")
		{
			usage(argv[0]);
			return 0;
		}
		else if (!a.empty() && a[0] == '-')
		{
			cerr << "未知选项: " << a << "\n";
			usage(argv[0]);
			return 1;
		}
		else
			opt.input = a;
	}
	if (opt.input.empty())
	{
		usage(argv[0]);
		return 1;
	}

	ifstream in(opt.input);
	if (!in)
	{
		cerr << "无法打开文件: " << opt.input << "\n";
		return 1;
	}

	// 自动识别格式：折叠栈的每行以数字结尾且包含 ';'，perf script 有以空白开头的栈帧行
	string fmt = opt.format;
	if (fmt == "auto")
	{
		string line;
		int folded = 0, script = 0, checked = 0;
		while (checked < 200 && getline(in, line))
		{
			if (trim(line).empty() || line[0] == '#')
				continue;
			checked++;
			if (line[0] == ' ' || line[0] == '\t')
				script++;
			else
			{
				string t = trim(line);
				size_t sp = t.find_last_of(' ');
				if (sp != string::npos && t.find(';') != string::npos && isNumTok(t.substr(sp + 1)))
					folded++;
			}
		}
		fmt = (folded > script) ? "folded" : "script";
		in.clear();
		in.seekg(0);
	}

	Dataset ds;
	bool ok = (fmt == "folded") ? parseFolded(in, ds, opt) : parsePerfScript(in, ds, opt);
	if (!ok)
	{
		cerr << "没有解析到任何样本。请确认输入是 perf script 输出或折叠栈文件";
		if (!opt.comm.empty())
			cerr << "，以及线程名 " << opt.comm << " 是否正确";
		cerr << "。\n";
		return 1;
	}

	if (!opt.foldedOut.empty())
		writeFolded(ds, opt);

	Analyzer an(ds, opt);
	an.run();
	return 0;
}
