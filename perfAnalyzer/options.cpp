// options.cpp
#include "options.h"

#include <cstdlib>
#include <iostream>

using namespace std;

void usage(const char* prog) {
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

int parseOptions(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        auto next = [&]() -> string {
            if (i + 1 >= argc) { usage(argv[0]); exit(1); }
            return argv[++i];
        };
        if (a == "--format") opt.format = next();
        else if (a == "--duration") opt.duration = stod(next());
        else if (a == "--freq") opt.freq = stod(next());
        else if (a == "--top") opt.top = stoi(next());
        else if (a == "--min-pct") opt.minPct = stod(next());
        else if (a == "--comm") opt.comm = next();
        else if (a == "--folded-out") opt.foldedOut = next();
        else if (a == "--keep-root-anomalies") opt.keepRootAnomalies = true;
        else if (a == "--mark-kernel") opt.markKernel = true;
        else if (a == "-h" || a == "--help") { usage(argv[0]); return -1; }
        else if (!a.empty() && a[0] == '-') { cerr << "未知选项: " << a << "\n"; usage(argv[0]); return 1; }
        else opt.input = a;
    }
    if (opt.input.empty()) { usage(argv[0]); return 1; }
    return 0;
}
