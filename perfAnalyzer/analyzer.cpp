// analyzer.cpp
#include "analyzer.h"
#include "string_utils.h"
#include "symbol_rules.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;

void Analyzer::run()
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

const string& Analyzer::nm(int id) const { return ds.names.str(id); }

bool Analyzer::isApp(const Stack& s, size_t i) const { return i > 0 && !s.k[i] && !isLibName(nm(s.f[i])); }

void Analyzer::aggregate()
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
		// Syscalls: find the first syscall entry frame; the nearest user-space frame above it is the wrapper
		for (size_t i = 1; i < s.f.size(); i++)
		{
			if (s.k[i] && isSyscallEntry(nm(s.f[i])))
			{
				syscallW += s.w;
				string wrapper = "[unknown]";
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

double Analyzer::inclOf(const set<string>& targets, const string& thread) const
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

vector<pair<string, double>> Analyzer::appCallers(const set<string>& targets, size_t n) const
{
	map<string, double> m;
	for (const auto& s : ds.stacks)
	{
		for (size_t i = s.f.size(); i-- > 1;)
		{
			if (!targets.count(stripVersion(nm(s.f[i]))))
				continue;
			string caller = "(caller unknown, unwinding stopped in a library or at the stack bottom)";
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

vector<pair<string, double>> Analyzer::byThread(const set<string>& targets) const
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

void Analyzer::printHeader() const
{
	cout << "# perf Sampling Data Analysis Report\n\n";
	cout << "- Input file: " << opt.input << " (" << ds.inputFormat << ")\n";
	if (!ds.event.empty())
		cout << "- Sampling event: " << ds.event << "\n";
	if (!opt.comm.empty())
		cout << "- Thread analyzed: " << opt.comm << "\n";
	cout << "\n";
}

void Analyzer::printQuality() const
{
	cout << "## 1. Data Quality Checks\n\n";
	vector<string> issues;

	if (W < 300)
		issues.push_back("Only " + numStr(W) +
						 " samples; statistical significance is weak and items below 2% are essentially noise. "
						 "Consider raising the sampling frequency (-F 199), capturing for longer, and capturing "
						 "while the workload is busy.");
	else if (W < 1000)
		issues.push_back(numStr(W) + " samples is on the low side; conclusions are only directional. "
						 "Aim for several thousand samples or more.");

	// Unresolved frames
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
		os << "Share of samples whose function name could not be resolved: " << pctStr(unresolvedLeaf, W) << ", mainly: ";
		vector<pair<string, double>> v(unresolvedBy.begin(), unresolvedBy.end());
		sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
		for (size_t i = 0; i < v.size() && i < 4; i++)
			os << (i ? ", " : "") << v[i].first << " (" << pctStr(v[i].second, W) << ")";
		os << ". Showing only the library name means the file is missing from --symfs, is stripped, or its path "
			  "differs from the one on the board; static functions inside a stripped library cannot be recovered anyway.";
		issues.push_back(os.str());
	}

	// libc version mismatch
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
			list += (list.empty() ? "" : ", ") + h;
		issues.push_back("libc functions that are normally never hot appeared: " + list + " (total " +
						 pctStr(suspicious, W) +
						 "). The libc in symfs most likely differs from the version on the board, so samples were "
						 "attributed to the wrong function names. Use the libc copied directly from the board, or "
						 "compare the Build IDs of the two.");
	}

	// Anomaly frames at the stack bottom
	if (W > 0 && ds.anomalyStripped / W > 0.3)
		issues.push_back(
			pctStr(ds.anomalyStripped, W) +
			" of call stacks have [perf-N.map]/[JIT]/[unknown] frames at the bottom. This happens when frame-pointer "
			"unwinding runs past the stack bottom into executable anonymous memory (e.g. an executable thread stack). "
			"These frames were removed from the analysis and do not affect the hotspots above them.");

	// Broken unwinding: only one user-space frame
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
						 " of samples have only one user-space frame, so the caller is lost. Common causes are "
						 "samples landing in library functions without frame pointers (e.g. libc qsort callbacks, "
						 "memcpy), or the program not being built with -marm -mapcs-frame (ARM32) / "
						 "-fno-omit-frame-pointer.");

	// Implausible call chains cannot be detected automatically; print a hint
	issues.push_back("The tool cannot tell whether application call chains are plausible: if you see call "
					 "relationships that clearly contradict the program logic, the binary used for symbol resolution "
					 "differs from the one running on the board. Verify with md5sum before analyzing.");

	for (auto& s : issues)
		cout << "- " << s << "\n";
	cout << "\n";
}

void Analyzer::printOverview() const
{
	cout << "## 2. Overview\n\n";
	cout << "| Metric | Value |\n| --- | --- |\n";
	cout << "| Samples | " << numStr(W) << " |\n";
	double cpuSec = -1;
	if (ds.periodIsNs && ds.periodSum > 0)
		cpuSec = ds.periodSum / 1e9;
	else if (opt.freq > 0)
		cpuSec = W / opt.freq;
	if (cpuSec >= 0)
		cout << "| CPU time | about " << numStr(cpuSec, 2) << " s |\n";
	if (cpuSec >= 0 && opt.duration > 0)
		cout << "| Average CPU usage | about " << numStr(100.0 * cpuSec / opt.duration, 1) << "% of one core ("
			 << numStr(opt.duration) << " s capture) |\n";
	cout << "| Samples in kernel | " << pctStr(kernelLeafW, W) << " |\n";
	cout << "| Samples in syscalls | " << pctStr(syscallW, W) << " |\n";
	cout << "| Threads | " << threadW.size() << " |\n\n";

	if (cpuSec >= 0 && opt.duration > 0 && cpuSec / opt.duration < 0.2)
		cout << "Overall process CPU usage is low. The hotspots below are relative shares, so optimizing them "
				"yields limited absolute CPU savings; to investigate high load or stalls, capture while the "
				"problem is occurring.\n\n";
	if (cpuSec >= 0 && opt.duration <= 0)
		cout << "Hint: add --duration <capture seconds> to compute average CPU usage.\n\n";

	if (!syscallWrapperW.empty())
	{
		vector<pair<string, double>> v(syscallWrapperW.begin(), syscallWrapperW.end());
		sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
		cout << "Syscalls by user-space function:\n\n| Function | Share of all samples |\n| --- | --- |\n";
		for (size_t i = 0; i < v.size() && i < 8; i++)
			cout << "| " << pretty(v[i].first) << " | " << pctStr(v[i].second, W) << " |\n";
		cout << "\n";
	}
}

void Analyzer::printThreads() const
{
	cout << "## 3. Thread Distribution\n\n";
	vector<pair<string, double>> v(threadW.begin(), threadW.end());
	sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
	cout << "| Thread | Share | Top application functions (inclusive, share of all samples) |\n| --- | --- | --- |\n";
	for (auto& t : v)
	{
		if (t.second / W * 100 < 0.5)
			continue;
		// Application functions with the highest inclusive share in this thread
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
			tops += (tops.empty() ? "" : "; ") + pretty(n, 60) + " " + pctStr(p.second, W);
			shown++;
		}
		cout << "| " << t.first << " | " << pctStr(t.second, W) << " | " << (tops.empty() ? "—" : tops) << " |\n";
	}
	cout << "\n";

	// Hottest call path of the main threads
	cout << "Hottest call paths of the main threads (application functions, caller above callee):\n\n";
	int shownThreads = 0;
	for (auto& t : v)
	{
		if (shownThreads >= 3 || t.second / W < 0.08)
			break;
		printHotPath(t.first, t.second);
		shownThreads++;
	}
}

void Analyzer::printHotPath(const string& thread, double threadWeight) const
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
	cout << "**" << thread << "** (" << pctStr(threadWeight, W) << ")\n\n```\n";
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

void Analyzer::printTopLists() const
{
	cout << "## 4. Hotspot Rankings\n\n";
	vector<int> ids(N);
	for (size_t i = 0; i < N; i++)
		ids[i] = (int)i;

	cout << "### Self time (samples landing directly in the function)\n\n| Rank | Function | Share | Type |\n| --- | --- | --- | --- |\n";
	sort(ids.begin(), ids.end(), [&](int a, int b) { return selfW[a] > selfW[b]; });
	for (int r = 0, i = 0; i < (int)ids.size() && r < opt.top; i++)
	{
		if (selfW[ids[i]] <= 0)
			break;
		cout << "| " << ++r << " | " << pretty(nm(ids[i])) << " | " << pctStr(selfW[ids[i]], W) << " | "
			 << kindOf(ids[i]) << " |\n";
	}
	cout << "\n### Inclusive time (including callees, application functions only)\n\n| Rank | Function | Share |\n| --- | --- | --- |\n";
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

bool Analyzer::isKernelId(int id) const
{
	for (const auto& s : ds.stacks)
		for (size_t i = 1; i < s.f.size(); i++)
			if (s.f[i] == id)
				return s.k[i];
	return false;
}

string Analyzer::kindOf(int id) const
{
	const string& n = nm(id);
	if (isKernelId(id))
		return "kernel";
	if (isUnresolved(n))
		return "unresolved";
	if (isLibName(n))
		return "library/runtime";
	return "application";
}

void Analyzer::addCallerDetails(Finding& f, const set<string>& targets) const
{
	for (auto& c : appCallers(targets))
		f.details.push_back("Caller " + pretty(c.first, 90) + ": " + pctStr(c.second, W));
	auto bt = byThread(targets);
	if (!bt.empty())
	{
		string s = "Threads: ";
		for (size_t i = 0; i < bt.size() && i < 4; i++)
			s += (i ? ", " : "") + bt[i].first + " " + pctStr(bt[i].second, W);
		f.details.push_back(s);
	}
}

bool Analyzer::over(double w) const { return W > 0 && 100.0 * w / W >= opt.minPct; }

void Analyzer::detect()
{
	// 1. gettid
	{
		double w = inclOf({"gettid", "__gettid"});
		if (over(w))
		{
			Finding f;
			f.title = "Frequent gettid calls to get the thread ID";
			f.weight = w;
			addCallerDetails(f, {"gettid", "__gettid"});
			f.advice = "Every gettid call is a syscall. Get it once at thread entry and cache it (local variable or "
					   "thread_local) instead of calling it repeatedly in loops, logging or locking paths.";
			f.effort = "trivial";
			findings.push_back(f);
		}
	}
	// 2. Getting the time
	{
		set<string> t = {"clock_gettime", "__clock_gettime", "gettimeofday", "__gettimeofday", "time"};
		double w = inclOf(t);
		if (over(w))
		{
			Finding f;
			f.title = "Frequently getting the system time";
			f.weight = w;
			addCallerDetails(f, t);
			double sys = 0;
			for (auto& p : syscallWrapperW)
				if (t.count(stripVersion(p.first)))
					sys += p.second;
			if (sys > 0)
				f.details.push_back(
					"Part that entered the kernel: " + pctStr(sys, W) +
					" (vDSO fast path not used; common for 32-bit programs on a 64-bit kernel)");
			f.advice = "Get the time once per processing cycle and reuse it; use a cached timestamp where precision is not critical.";
			f.effort = "small";
			findings.push_back(f);
		}
	}
	// 3. 64-bit software division
	{
		set<string> t = {"__udivmoddi4", "__aeabi_uldivmod", "__aeabi_ldivmod", "__divdi3", "__udivdi3", "__moddi3",
			"__umoddi3", "__divmoddi4"};
		double w = inclOf(t);
		if (over(w))
		{
			Finding f;
			f.title = "64-bit integer software division (__udivmoddi4, etc.)";
			f.weight = w;
			addCallerDetails(f, t);
			f.advice = "32-bit ARM has no 64-bit divide instruction, so / and % on int64/uint64 are done in software and "
					   "are expensive. This is common in time unit conversions: convert only once per cycle, approximate "
					   "with shifts or multiplication, or keep a single time unit to avoid division.";
			f.effort = "small";
			findings.push_back(f);
		}
	}
	// 4. Sort comparison functions (qsort callbacks, etc.)
	for (int id = 0; id < (int)N; id++)
	{
		const string& n = nm(id);
		string low = toLower(n);
		bool isCmp = (contains(low, "compare") || contains(low, "cmp")) && !startsWith(low, "memcmp") &&
					 !startsWith(low, "strcmp") && !startsWith(low, "strncmp") && !contains(low, "strcasecmp") &&
					 !isLibName(n);
		if (!isCmp || !over(selfW[id]))
			continue;
		// Classify callers: library function / stack bottom -> callback-style sorting
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
		// Samples in unresolved libc-internal functions on the same thread (the sort algorithm itself)
		double libInternal = 0;
		for (const auto& s : ds.stacks)
			if (nm(s.f[0]) == mainThread && startsWith(nm(s.f.back()), "[libc"))
				libInternal += s.w;
		Finding f;
		f.title = "Comparison function " + pretty(n, 70) + " called frequently (likely repeated sorting)";
		f.weight = inclW[id] + libInternal;
		f.details.push_back("Comparison function self: " + pctStr(selfW[id], W));
		if (total > 0)
			f.details.push_back("Share called from library functions or broken stacks: " + pctStr(viaLib, total).substr(0) +
								" (typical of callbacks from library functions such as qsort)");
		if (!mainThread.empty())
		{
			double tw = threadW.count(mainThread) ? threadW.at(mainThread) : 0;
			f.details.push_back("Mainly on thread " + mainThread + " (thread total " + pctStr(tw, W) +
								"); libc-internal functions on the same thread " + pctStr(libInternal, W) +
								"; comparison function + sort body take about this much of the thread: " +
								(tw > 0 ? pctStr(selfW[id] + libInternal, tw) : string("?")));
		}
		f.advice = "If qsort/std::sort is called on the whole list every cycle, switch to a min-heap "
				   "(std::priority_queue) or sorted insertion and only handle new and expired elements; also check "
				   "the comparison function itself for unnecessary overhead.";
		f.effort = "medium";
		findings.push_back(f);
	}
	// 5. map/unordered_map keyed by strings
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
			f.title = "std::map lookups keyed by strings and string operations";
			f.weight = w + strW;
			f.details.push_back("Red-black tree/hash table (string keys): " + pctStr(w, W));
			f.details.push_back("String compare/length/construct/assign (memcmp, strlen, std::string, etc.) self: " + pctStr(strW, W));
			for (auto& c : appCallers(rbNames))
				f.details.push_back("Lookup caller " + pretty(c.first, 90) + ": " + pctStr(c.second, W));
			f.advice =
				"Attributes/configuration are looked up by name string on a hot path. Resolve names to indexes, enums "
				"or cached pointers at initialization and access them directly each cycle; avoid constructing "
				"temporary std::string objects for lookups (use std::string_view or the transparent comparator "
				"std::less<>).";
			f.effort = "medium";
			findings.push_back(f);
		}
	}
	// 6. select/poll/epoll/nanosleep polling
	{
		set<string> t = {"select", "__select", "pselect", "poll", "__poll", "ppoll", "epoll_wait", "epoll_pwait",
			"nanosleep", "__nanosleep", "clock_nanosleep", "usleep", "sleep"};
		double w = inclOf(t);
		if (over(w))
		{
			Finding f;
			f.title = "select/poll/sleep wait calls consuming CPU (likely short-timeout polling)";
			f.weight = w;
			for (auto& bt : byThread(t))
			{
				double tw = threadW.count(bt.first) ? threadW.at(bt.first) : 0;
				if (bt.second / W * 100 < opt.minPct)
					continue;
				f.details.push_back("Thread " + bt.first + ": " + pctStr(bt.second, W) + " (" +
									(tw > 0 ? pctStr(bt.second, tw) : string("?")) + " of the thread)");
			}
			int dsId = ds.names.find("do_select");
			if (dsId >= 0 && selfW[dsId] > 0)
				f.details.push_back("Kernel do_select self: " + pctStr(selfW[dsId], W));
			f.advice = "Blocking waits by themselves do not consume CPU; a noticeable share means the thread is woken up "
					   "frequently. Check whether select/poll timeouts are too short or whether sleep is used for polling "
					   "in a loop, and prefer longer timeouts or a purely event-driven design.";
			f.effort = "small";
			findings.push_back(f);
		}
	}
	// 7. Memory allocation
	{
		set<string> t = {"malloc", "free", "cfree", "realloc", "calloc", "_int_malloc", "_int_free", "_int_realloc",
			"_int_memalign", "malloc_consolidate", "operator new(unsigned int)", "operator new(unsigned long)",
			"operator delete(void*)", "operator new", "operator delete", "memalign"};
		double w = inclOf(t);
		if (over(w * 0.5) && over(w))
		{
			Finding f;
			f.title = "Frequent memory allocation/deallocation";
			f.weight = w;
			addCallerDetails(f, t);
			f.advice =
				"Check hot loops for temporary objects, std::string/std::vector copies and Json object construction; "
				"reduce allocations with reserve, object reuse, passing by reference or move.";
			f.effort = "medium";
			findings.push_back(f);
		}
	}
	// 8. Locks/synchronization
	{
		set<string> t = {"sem_wait", "sem_trywait", "sem_timedwait", "sem_post", "pthread_mutex_lock",
			"pthread_mutex_unlock", "__pthread_mutex_lock", "__pthread_mutex_unlock_usercnt", "pthread_cond_wait",
			"pthread_cond_timedwait", "__lll_lock_wait", "PosixSemTake", "PosixSemGive"};
		double w = inclOf(t);
		if (over(w))
		{
			Finding f;
			f.title = "Lock and semaphore operations";
			f.weight = w;
			addCallerDetails(f, t);
			f.advice = "Retrying non-blocking operations such as sem_trywait in a loop amounts to busy-waiting. Check "
					   "for polling-style locking, and review lock granularity and hold times.";
			f.effort = "medium";
			findings.push_back(f);
		}
	}
	// 9. JSON parsing
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
			f.title = "JSON parsing";
			f.weight = w;
			addCallerDetails(f, t);
			f.advice = "Check whether the same JSON string is parsed repeatedly on a periodic path; parse once and cache the result.";
			f.effort = "medium";
			findings.push_back(f);
		}
	}
	// 10. Overall syscall share
	if (W > 0 && syscallW / W > 0.2)
	{
		Finding f;
		f.title = "Overall syscall share is high";
		f.weight = syscallW;
		vector<pair<string, double>> v(syscallWrapperW.begin(), syscallWrapperW.end());
		sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
		for (size_t i = 0; i < v.size() && i < 5; i++)
			f.details.push_back(pretty(v[i].first) + ": " + pctStr(v[i].second, W));
		f.advice =
			"Entering the kernel has a fixed cost; this share will drop noticeably once the top call types above "
			"(gettid, polling, getting the time) are addressed.";
		f.effort = "—";
		findings.push_back(f);
	}

	sort(findings.begin(), findings.end(),
		[](const Finding& a, const Finding& b)
		{
			// "Overall syscall share" is a summary item, put it last
			bool as = startsWith(a.title, "Overall syscall"), bs = startsWith(b.title, "Overall syscall");
			if (as != bs)
				return bs;
			return a.weight > b.weight;
		});
}

void Analyzer::printFindings() const
{
	cout << "## 5. Rule Check Results\n\n";
	if (findings.empty())
	{
		cout << "No rules matched (threshold " << opt.minPct << "%).\n\n";
		return;
	}
	int i = 0;
	for (const auto& f : findings)
	{
		cout << "### " << ++i << ". " << f.title << " (about " << pctStr(f.weight, W) << ")\n\n";
		for (const auto& d : f.details)
			cout << "- " << d << "\n";
		cout << "\nRecommendation: " << f.advice << "\n\n";
	}

	cout << "## 6. Optimization Priorities\n\n| Priority | Issue | Share | Effort |\n| --- | --- | --- | --- |\n";
	// Sort by "share / effort": lower effort gets higher priority
	auto cost = [](const string& e) { return e == "trivial" ? 1.0 : e == "small" ? 1.5 : e == "medium" ? 2.5 : 4.0; };
	vector<const Finding*> v;
	for (auto& f : findings)
		if (f.effort != "—")
			v.push_back(&f);
	sort(v.begin(), v.end(),
		[&](const Finding* a, const Finding* b) { return a->weight / cost(a->effort) > b->weight / cost(b->effort); });
	int r = 0;
	for (auto* f : v)
		cout << "| " << ++r << " | " << f->title << " | " << pctStr(f->weight, W) << " | " << f->effort << " |\n";
	cout << "\n";
}

void Analyzer::printCaveats() const
{
	cout << "## 7. Notes\n\n";
	cout << "- All shares are relative to all samples; rules may overlap (e.g. gettid also counts toward the overall syscall share).\n";
	cout << "- A CPU flame graph only reflects on-CPU time; time spent blocked, waiting for locks or waiting for IO does not appear here.\n";
	if (ds.inputFormat == "folded stacks")
		cout << "- With folded stack input, kernel functions are identified heuristically by name; use perf script "
				"output as input for an accurate kernel/user-space split.\n";
	cout << "\n";
}
