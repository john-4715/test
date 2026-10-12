// analyzer.cpp
#include "analyzer.h"
#include "string_utils.h"
#include "symbol_rules.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;

void Analyzer::run() {
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

const string& Analyzer::nm(int id) const {
    return ds.names.str(id);
}

bool Analyzer::isApp(const Stack& s, size_t i) const {
    return i > 0 && !s.k[i] && !isLibName(nm(s.f[i]));
}

void Analyzer::aggregate() {
    W = ds.totalSamples;
    selfW.assign(N, 0);
    inclW.assign(N, 0);
    vector<int> stamp(N, -1);
    for (size_t si = 0; si < ds.stacks.size(); si++) {
        const Stack& s = ds.stacks[si];
        threadW[nm(s.f[0])] += s.w;
        selfW[s.f.back()] += s.w;
        for (size_t i = 1; i < s.f.size(); i++) {
            int id = s.f[i];
            if (stamp[id] != (int)si) {
                stamp[id] = (int)si;
                inclW[id] += s.w;
            }
        }
        if (s.k.back()) kernelLeafW += s.w;
        // 系统调用：找到第一个系统调用入口帧，它上方最近的用户态帧即包装函数
        for (size_t i = 1; i < s.f.size(); i++) {
            if (s.k[i] && isSyscallEntry(nm(s.f[i]))) {
                syscallW += s.w;
                string wrapper = "[未知]";
                for (size_t j = i; j-- > 1;) {
                    if (!s.k[j] && !isRootAnomaly(nm(s.f[j]))) { wrapper = nm(s.f[j]); break; }
                }
                syscallWrapperW[wrapper] += s.w;
                break;
            }
        }
    }
}

double Analyzer::inclOf(const set<string>& targets, const string& thread) const {
    double w = 0;
    for (const auto& s : ds.stacks) {
        if (!thread.empty() && nm(s.f[0]) != thread) continue;
        for (size_t i = 1; i < s.f.size(); i++)
            if (targets.count(stripVersion(nm(s.f[i])))) { w += s.w; break; }
    }
    return w;
}

vector<pair<string, double>> Analyzer::appCallers(const set<string>& targets, size_t n) const {
    map<string, double> m;
    for (const auto& s : ds.stacks) {
        for (size_t i = s.f.size(); i-- > 1;) {
            if (!targets.count(stripVersion(nm(s.f[i])))) continue;
            string caller = "（调用者未知，回溯在库或栈底中断）";
            for (size_t j = i; j-- > 1;)
                if (isApp(s, j) && !targets.count(stripVersion(nm(s.f[j])))) { caller = nm(s.f[j]); break; }
            m[caller] += s.w;
            break;
        }
    }
    vector<pair<string, double>> v(m.begin(), m.end());
    sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
    if (v.size() > n) v.resize(n);
    return v;
}

vector<pair<string, double>> Analyzer::byThread(const set<string>& targets) const {
    map<string, double> m;
    for (const auto& s : ds.stacks)
        for (size_t i = 1; i < s.f.size(); i++)
            if (targets.count(stripVersion(nm(s.f[i])))) { m[nm(s.f[0])] += s.w; break; }
    vector<pair<string, double>> v(m.begin(), m.end());
    sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
    return v;
}

void Analyzer::printHeader() const {
    cout << "# perf 采样数据分析报告\n\n";
    cout << "- 输入文件：" << opt.input << "（" << ds.inputFormat << "）\n";
    if (!ds.event.empty()) cout << "- 采样事件：" << ds.event << "\n";
    if (!opt.comm.empty()) cout << "- 只分析线程：" << opt.comm << "\n";
    cout << "\n";
}

void Analyzer::printQuality() const {
    cout << "## 一、数据质量检查\n\n";
    vector<string> issues;

    if (W < 300)
        issues.push_back("样本数只有 " + numStr(W) + " 个，统计意义很弱，小于 2% 的项基本可视为噪声。建议提高采样频率（-F 199）、延长采集时间，并在业务繁忙时采集。");
    else if (W < 1000)
        issues.push_back("样本数 " + numStr(W) + " 个，偏少，结论只能作为方向参考，建议采集到几千个样本以上。");

    // 未解析帧
    double unresolvedLeaf = 0;
    map<string, double> unresolvedBy;
    for (const auto& s : ds.stacks) {
        const string& leaf = nm(s.f.back());
        if (isUnresolved(leaf) && !s.k.back()) {
            unresolvedLeaf += s.w;
            unresolvedBy[leaf] += s.w;
        }
    }
    if (W > 0 && unresolvedLeaf / W > 0.05) {
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
    for (int id = 0; id < (int)N; id++) {
        string n = stripVersion(nm(id));
        if (kSuspiciousLibc.count(n) && inclW[id] > 0) { suspicious += inclW[id]; hit.insert(n); }
    }
    if (W > 0 && suspicious / W > 0.01) {
        string list;
        for (auto& h : hit) list += (list.empty() ? "" : "、") + h;
        issues.push_back("出现了 " + list + " 等通常不可能成为热点的 libc 函数（合计 " + pctStr(suspicious, W) +
                         "），很可能是 symfs 中的 libc 与板子上的版本不一致，函数名被\"对错号\"。"
                         "请直接使用从板子上拷贝的 libc，或核对两者的 Build ID。");
    }

    // 栈底异常帧
    if (W > 0 && ds.anomalyStripped / W > 0.3)
        issues.push_back(pctStr(ds.anomalyStripped, W) + " 的调用栈底部是 [perf-N.map]/[JIT]/[unknown] 帧，"
                         "这是帧指针回溯越过栈底、落到可执行匿名内存（如可执行线程栈）造成的，已在分析中去除，不影响上层热点判断。");

    // 回溯断裂：用户态只有一层
    double shallow = 0;
    for (const auto& s : ds.stacks) {
        int userFrames = 0;
        for (size_t i = 1; i < s.f.size(); i++) if (!s.k[i]) userFrames++;
        if (userFrames <= 1) shallow += s.w;
    }
    if (W > 0 && shallow / W > 0.25)
        issues.push_back(pctStr(shallow, W) + " 的样本用户态只有一层栈帧，调用者丢失。常见原因是采样点位于没有帧指针的库函数中"
                         "（如 libc 的 qsort 回调、memcpy），或程序未用 -marm -mapcs-frame（ARM32）/ -fno-omit-frame-pointer 编译。");

    // 不合理调用链无法自动判定，给出提示
    issues.push_back("工具无法自动判断业务调用链是否合理：如果看到明显不符合业务逻辑的调用关系，"
                     "说明解析用的程序与板子上运行的版本不一致，请用 md5sum 核对后再分析。");

    for (auto& s : issues) cout << "- " << s << "\n";
    cout << "\n";
}

void Analyzer::printOverview() const {
    cout << "## 二、整体概况\n\n";
    cout << "| 指标 | 数值 |\n| --- | --- |\n";
    cout << "| 样本数 | " << numStr(W) << " |\n";
    double cpuSec = -1;
    if (ds.periodIsNs && ds.periodSum > 0) cpuSec = ds.periodSum / 1e9;
    else if (opt.freq > 0) cpuSec = W / opt.freq;
    if (cpuSec >= 0) cout << "| CPU 时间 | 约 " << numStr(cpuSec, 2) << " 秒 |\n";
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

    if (!syscallWrapperW.empty()) {
        vector<pair<string, double>> v(syscallWrapperW.begin(), syscallWrapperW.end());
        sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
        cout << "系统调用按用户态函数分布：\n\n| 函数 | 占全部样本 |\n| --- | --- |\n";
        for (size_t i = 0; i < v.size() && i < 8; i++)
            cout << "| " << pretty(v[i].first) << " | " << pctStr(v[i].second, W) << " |\n";
        cout << "\n";
    }
}

void Analyzer::printThreads() const {
    cout << "## 三、线程分布\n\n";
    vector<pair<string, double>> v(threadW.begin(), threadW.end());
    sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
    cout << "| 线程 | 占比 | 主要业务函数（累计占全部样本） |\n| --- | --- | --- |\n";
    for (auto& t : v) {
        if (t.second / W * 100 < 0.5) continue;
        // 该线程内累计占比最高的几个业务函数
        map<int, double> inc;
        for (const auto& s : ds.stacks) {
            if (nm(s.f[0]) != t.first) continue;
            set<int> seen;
            for (size_t i = 1; i < s.f.size(); i++)
                if (isApp(s, i) && seen.insert(s.f[i]).second) inc[s.f[i]] += s.w;
        }
        vector<pair<int, double>> iv(inc.begin(), inc.end());
        sort(iv.begin(), iv.end(), [](auto& a, auto& b) { return a.second > b.second; });
        string tops;
        int shown = 0;
        for (auto& p : iv) {
            if (shown >= 3) break;
            const string& n = nm(p.first);
            if (n == "PosixTaskWrapper" || n == "PosixTaskSpawn") continue;
            tops += (tops.empty() ? "" : "；") + pretty(n, 60) + " " + pctStr(p.second, W);
            shown++;
        }
        cout << "| " << t.first << " | " << pctStr(t.second, W) << " | " << (tops.empty() ? "—" : tops) << " |\n";
    }
    cout << "\n";

    // 主要线程的最热调用路径
    cout << "主要线程的最热调用路径（业务函数，从上到下为调用关系）：\n\n";
    int shownThreads = 0;
    for (auto& t : v) {
        if (shownThreads >= 3 || t.second / W < 0.08) break;
        printHotPath(t.first, t.second);
        shownThreads++;
    }
}

void Analyzer::printHotPath(const string& thread, double threadWeight) const {
    struct Node { double w = 0; map<int, Node> ch; };
    Node root;
    for (const auto& s : ds.stacks) {
        if (nm(s.f[0]) != thread) continue;
        Node* cur = &root;
        cur->w += s.w;
        for (size_t i = 1; i < s.f.size(); i++) {
            if (!isApp(s, i)) continue;
            cur = &cur->ch[s.f[i]];
            cur->w += s.w;
        }
    }
    cout << "**" << thread << "**（" << pctStr(threadWeight, W) << "）\n\n```\n";
    const Node* cur = &root;
    int depth = 0;
    while (!cur->ch.empty() && depth < 10) {
        const pair<const int, Node>* best = nullptr;
        for (auto& c : cur->ch)
            if (!best || c.second.w > best->second.w) best = &c;
        if (best->second.w / W < 0.02) break;
        cout << string(depth * 2, ' ') << pretty(nm(best->first), 90) << "  " << pctStr(best->second.w, W) << "\n";
        cur = &best->second;
        depth++;
    }
    cout << "```\n\n";
}

void Analyzer::printTopLists() const {
    cout << "## 四、热点排行\n\n";
    vector<int> ids(N);
    for (size_t i = 0; i < N; i++) ids[i] = (int)i;

    cout << "### 自身耗时（采样点直接落在该函数）\n\n| 排名 | 函数 | 占比 | 类型 |\n| --- | --- | --- | --- |\n";
    sort(ids.begin(), ids.end(), [&](int a, int b) { return selfW[a] > selfW[b]; });
    for (int r = 0, i = 0; i < (int)ids.size() && r < opt.top; i++) {
        if (selfW[ids[i]] <= 0) break;
        cout << "| " << ++r << " | " << pretty(nm(ids[i])) << " | " << pctStr(selfW[ids[i]], W) << " | "
             << kindOf(ids[i]) << " |\n";
    }
    cout << "\n### 累计耗时（包含其调用的函数，只列业务函数）\n\n| 排名 | 函数 | 占比 |\n| --- | --- | --- |\n";
    sort(ids.begin(), ids.end(), [&](int a, int b) { return inclW[a] > inclW[b]; });
    for (int r = 0, i = 0; i < (int)ids.size() && r < opt.top; i++) {
        const string& n = nm(ids[i]);
        if (inclW[ids[i]] <= 0) break;
        if (isLibName(n) || threadW.count(n) || isKernelId(ids[i])) continue;
        if (n == "PosixTaskWrapper" || n == "PosixTaskSpawn") continue;
        cout << "| " << ++r << " | " << pretty(n) << " | " << pctStr(inclW[ids[i]], W) << " |\n";
    }
    cout << "\n";
}

bool Analyzer::isKernelId(int id) const {
    for (const auto& s : ds.stacks)
        for (size_t i = 1; i < s.f.size(); i++)
            if (s.f[i] == id) return s.k[i];
    return false;
}

string Analyzer::kindOf(int id) const {
    const string& n = nm(id);
    if (isKernelId(id)) return "内核";
    if (isUnresolved(n)) return "未解析";
    if (isLibName(n)) return "库/运行时";
    return "业务";
}

void Analyzer::addCallerDetails(Finding& f, const set<string>& targets) const {
    for (auto& c : appCallers(targets))
        f.details.push_back("调用者 " + pretty(c.first, 90) + "：" + pctStr(c.second, W));
    auto bt = byThread(targets);
    if (!bt.empty()) {
        string s = "所在线程：";
        for (size_t i = 0; i < bt.size() && i < 4; i++)
            s += (i ? "、" : "") + bt[i].first + " " + pctStr(bt[i].second, W);
        f.details.push_back(s);
    }
}

bool Analyzer::over(double w) const {
    return W > 0 && 100.0 * w / W >= opt.minPct;
}

void Analyzer::detect() {
    // 1. gettid
    {
        double w = inclOf({"gettid", "__gettid"});
        if (over(w)) {
            Finding f;
            f.title = "频繁调用 gettid 获取线程号";
            f.weight = w;
            addCallerDetails(f, {"gettid", "__gettid"});
            f.advice = "gettid 每次都是一次系统调用。在线程入口获取一次并缓存（局部变量或 thread_local），不要在循环、日志或加锁路径中反复调用。";
            f.effort = "很小";
            findings.push_back(f);
        }
    }
    // 2. 取时间
    {
        set<string> t = {"clock_gettime", "__clock_gettime", "gettimeofday", "__gettimeofday", "time"};
        double w = inclOf(t);
        if (over(w)) {
            Finding f;
            f.title = "频繁获取系统时间";
            f.weight = w;
            addCallerDetails(f, t);
            double sys = 0;
            for (auto& p : syscallWrapperW) if (t.count(stripVersion(p.first))) sys += p.second;
            if (sys > 0) f.details.push_back("其中进入内核的部分：" + pctStr(sys, W) + "（未走 vDSO 快速路径，64 位内核上的 32 位程序常见）");
            f.advice = "一个处理周期内只取一次时间并复用；对精度要求不高的地方可以用缓存的时间戳。";
            f.effort = "小";
            findings.push_back(f);
        }
    }
    // 3. 64 位软件除法
    {
        set<string> t = {"__udivmoddi4", "__aeabi_uldivmod", "__aeabi_ldivmod", "__divdi3",
                         "__udivdi3", "__moddi3", "__umoddi3", "__divmoddi4"};
        double w = inclOf(t);
        if (over(w)) {
            Finding f;
            f.title = "64 位整数软件除法（__udivmoddi4 等）";
            f.weight = w;
            addCallerDetails(f, t);
            f.advice = "32 位 ARM 没有 64 位除法指令，int64/uint64 的 / 和 % 由软件实现，开销较大。常见于时间单位换算，"
                       "可在一个周期内只换算一次、改用移位或乘法近似，或直接保持统一的时间单位避免除法。";
            f.effort = "小";
            findings.push_back(f);
        }
    }
    // 4. 排序比较函数（qsort 回调等）
    for (int id = 0; id < (int)N; id++) {
        const string& n = nm(id);
        string low = toLower(n);
        bool isCmp = (contains(low, "compare") || contains(low, "cmp")) &&
                     !startsWith(low, "memcmp") && !startsWith(low, "strcmp") &&
                     !startsWith(low, "strncmp") && !contains(low, "strcasecmp") && !isLibName(n);
        if (!isCmp || !over(selfW[id])) continue;
        // 统计调用者类型：库函数/栈底 -> 回调式排序
        double viaLib = 0, total = 0;
        map<string, double> thr;
        for (const auto& s : ds.stacks) {
            for (size_t i = s.f.size(); i-- > 1;) {
                if (s.f[i] != id) continue;
                total += s.w;
                thr[nm(s.f[0])] += s.w;
                if (i == 1 || isLibName(nm(s.f[i - 1])) || isRootAnomaly(nm(s.f[i - 1]))) viaLib += s.w;
                break;
            }
        }
        string mainThread;
        double mt = 0;
        for (auto& p : thr) if (p.second > mt) { mt = p.second; mainThread = p.first; }
        // 同线程中 libc 内部未解析函数的样本（排序算法本体）
        double libInternal = 0;
        for (const auto& s : ds.stacks)
            if (nm(s.f[0]) == mainThread && startsWith(nm(s.f.back()), "[libc")) libInternal += s.w;
        Finding f;
        f.title = "比较函数 " + pretty(n, 70) + " 被频繁调用（疑似反复排序）";
        f.weight = inclW[id] + libInternal;
        f.details.push_back("比较函数自身：" + pctStr(selfW[id], W));
        if (total > 0)
            f.details.push_back("其中由库函数或断裂栈调用的比例：" + pctStr(viaLib, total).substr(0) + "（qsort 等库函数回调的典型特征）");
        if (!mainThread.empty()) {
            double tw = threadW.count(mainThread) ? threadW.at(mainThread) : 0;
            f.details.push_back("主要在线程 " + mainThread + "（该线程共 " + pctStr(tw, W) + "）；同线程 libc 内部函数 " +
                                pctStr(libInternal, W) + "，比较函数 + 排序本体约占该线程的 " +
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
        for (int id = 0; id < (int)N; id++) {
            const string& n = nm(id);
            bool rb = (contains(n, "_Rb_tree<std::__cxx11::basic_string") || contains(n, "_Rb_tree<std::string") ||
                       contains(n, "map<std::__cxx11::basic_string") || contains(n, "_Hashtable<std::__cxx11::basic_string"));
            if (rb) rbNames.insert(n);
        }
        if (!rbNames.empty()) w = inclOf(rbNames);
        set<string> strOps = {"memcmp", "strcmp", "strlen", "strncmp"};
        double strW = 0;
        for (int id = 0; id < (int)N; id++) {
            const string& n = stripVersion(nm(id));
            if (strOps.count(n) || (startsWith(n, "std::__cxx11::basic_string") &&
                (contains(n, "_M_construct") || contains(n, "_M_replace") || contains(n, "_M_assign") ||
                 contains(n, "compare") || contains(n, "basic_string("))) ||
                contains(n, "basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct"))
                strW += selfW[id];
        }
        if (over(w) || (over(strW) && !rbNames.empty())) {
            Finding f;
            f.title = "以字符串为键的 std::map 查找和字符串操作";
            f.weight = w + strW;
            f.details.push_back("红黑树/哈希表（字符串键）相关：" + pctStr(w, W));
            f.details.push_back("字符串比较/长度/构造/赋值（memcmp、strlen、std::string 等）自身：" + pctStr(strW, W));
            for (auto& c : appCallers(rbNames))
                f.details.push_back("查找发起者 " + pretty(c.first, 90) + "：" + pctStr(c.second, W));
            f.advice = "热点路径上按名字字符串查找属性/配置。可在初始化时把名字解析成索引、枚举或指针缓存，周期内直接访问；"
                       "避免为查找临时构造 std::string（可用 std::string_view 或透明比较器 std::less<>）。";
            f.effort = "中";
            findings.push_back(f);
        }
    }
    // 6. select/poll/epoll/nanosleep 轮询
    {
        set<string> t = {"select", "__select", "pselect", "poll", "__poll", "ppoll", "epoll_wait",
                         "epoll_pwait", "nanosleep", "__nanosleep", "clock_nanosleep", "usleep", "sleep"};
        double w = inclOf(t);
        if (over(w)) {
            Finding f;
            f.title = "select/poll/sleep 等待类调用占用 CPU（疑似短超时轮询）";
            f.weight = w;
            for (auto& bt : byThread(t)) {
                double tw = threadW.count(bt.first) ? threadW.at(bt.first) : 0;
                if (bt.second / W * 100 < opt.minPct) continue;
                f.details.push_back("线程 " + bt.first + "：" + pctStr(bt.second, W) + "（占该线程 " +
                                    (tw > 0 ? pctStr(bt.second, tw) : string("?")) + "）");
            }
            int dsId = ds.names.find("do_select");
            if (dsId >= 0 && selfW[dsId] > 0) f.details.push_back("内核 do_select 自身：" + pctStr(selfW[dsId], W));
            f.advice = "阻塞等待本身不耗 CPU，占比明显说明线程被频繁唤醒。检查 select/poll 的超时参数是否过短、"
                       "是否在循环中配合 sleep 轮询，尽量改为较长超时或纯事件驱动。";
            f.effort = "小";
            findings.push_back(f);
        }
    }
    // 7. 内存分配
    {
        set<string> t = {"malloc", "free", "cfree", "realloc", "calloc", "_int_malloc", "_int_free",
                         "_int_realloc", "_int_memalign", "malloc_consolidate", "operator new(unsigned int)",
                         "operator new(unsigned long)", "operator delete(void*)", "operator new",
                         "operator delete", "memalign"};
        double w = inclOf(t);
        if (over(w * 0.5) && over(w)) {
            Finding f;
            f.title = "内存分配/释放较频繁";
            f.weight = w;
            addCallerDetails(f, t);
            f.advice = "检查热点循环中的临时对象、std::string/std::vector 拷贝和 Json 对象构造；可通过 reserve、对象复用、"
                       "传引用或 move 减少分配。";
            f.effort = "中";
            findings.push_back(f);
        }
    }
    // 8. 锁/同步
    {
        set<string> t = {"sem_wait", "sem_trywait", "sem_timedwait", "sem_post", "pthread_mutex_lock",
                         "pthread_mutex_unlock", "__pthread_mutex_lock", "__pthread_mutex_unlock_usercnt",
                         "pthread_cond_wait", "pthread_cond_timedwait", "__lll_lock_wait", "PosixSemTake",
                         "PosixSemGive"};
        double w = inclOf(t);
        if (over(w)) {
            Finding f;
            f.title = "锁和信号量操作";
            f.weight = w;
            addCallerDetails(f, t);
            f.advice = "sem_trywait 等非阻塞操作如果在循环中反复尝试，相当于忙等。检查是否存在轮询式加锁，以及锁的粒度和持有时间。";
            f.effort = "中";
            findings.push_back(f);
        }
    }
    // 9. JSON 解析
    {
        set<string> t;
        for (int id = 0; id < (int)N; id++) {
            const string& n = nm(id);
            if (startsWith(n, "Json::Reader::") || startsWith(n, "nlohmann::") || contains(n, "json_parse") ||
                startsWith(n, "json_tokener_"))
                t.insert(n);
        }
        double w = t.empty() ? 0 : inclOf(t);
        if (over(w)) {
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
    if (W > 0 && syscallW / W > 0.2) {
        Finding f;
        f.title = "系统调用整体占比偏高";
        f.weight = syscallW;
        vector<pair<string, double>> v(syscallWrapperW.begin(), syscallWrapperW.end());
        sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
        for (size_t i = 0; i < v.size() && i < 5; i++)
            f.details.push_back(pretty(v[i].first) + "：" + pctStr(v[i].second, W));
        f.advice = "进入内核本身有固定开销，处理掉上面占比最高的几类调用（gettid、轮询、取时间）后，这个比例会明显下降。";
        f.effort = "—";
        findings.push_back(f);
    }

    sort(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
        // "系统调用整体占比"是汇总项，放到最后
        bool as = startsWith(a.title, "系统调用整体"), bs = startsWith(b.title, "系统调用整体");
        if (as != bs) return bs;
        return a.weight > b.weight;
    });
}

void Analyzer::printFindings() const {
    cout << "## 五、规则检测结果\n\n";
    if (findings.empty()) {
        cout << "没有命中任何规则（阈值 " << opt.minPct << "%）。\n\n";
        return;
    }
    int i = 0;
    for (const auto& f : findings) {
        cout << "### " << ++i << ". " << f.title << "（约 " << pctStr(f.weight, W) << "）\n\n";
        for (const auto& d : f.details) cout << "- " << d << "\n";
        cout << "\n建议：" << f.advice << "\n\n";
    }

    cout << "## 六、优化优先级\n\n| 优先级 | 问题 | 占比 | 改动难度 |\n| --- | --- | --- | --- |\n";
    // 按"占比 / 难度"排序：难度越小越优先
    auto cost = [](const string& e) { return e == "很小" ? 1.0 : e == "小" ? 1.5 : e == "中" ? 2.5 : 4.0; };
    vector<const Finding*> v;
    for (auto& f : findings) if (f.effort != "—") v.push_back(&f);
    sort(v.begin(), v.end(), [&](const Finding* a, const Finding* b) {
        return a->weight / cost(a->effort) > b->weight / cost(b->effort);
    });
    int r = 0;
    for (auto* f : v)
        cout << "| " << ++r << " | " << f->title << " | " << pctStr(f->weight, W) << " | " << f->effort << " |\n";
    cout << "\n";
}

void Analyzer::printCaveats() const {
    cout << "## 七、说明\n\n";
    cout << "- 占比均相对于全部样本；不同规则之间可能有重叠（例如 gettid 同时计入系统调用整体占比）。\n";
    cout << "- CPU 火焰图只反映占用 CPU 的时间，线程阻塞、等待锁、等待 IO 的时间不会出现在这里。\n";
    if (ds.inputFormat == "折叠栈")
        cout << "- 折叠栈输入中内核函数是按名字启发式识别的；使用 perf script 输出作为输入可以得到准确的内核/用户态区分。\n";
    cout << "\n";
}
