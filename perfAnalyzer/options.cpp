// options.cpp
#include "options.h"

#include <cstdlib>
#include <iostream>

using namespace std;

void usage(const char* prog)
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
		 << "  --mark-kernel                折叠栈中内核帧加 _[k] 后缀\n"
		 << "\n"
		 << "ELF 符号解析（perf 无法解析用户态函数名时，由本工具读取 ELF 符号表解析）：\n"
		 << "  --symfs <目录>               ELF 文件路径前缀，在板子上直接解析时不需要\n"
		 << "  --maps <文件>                /proc/PID/maps 格式的内存映射文件\n"
		 << "  --no-elf                     关闭 ELF 符号解析\n"
		 << "  建议：perf script -i perf.data --show-mmap-events > out.perf\n"
		 << "\n"
		 << "地址模式（板子上没有符号时，先记录地址，之后在主机上换算成函数名）：\n"
		 << "  --addr                       无法解析函数名时显示 \"文件名+0x链接地址\"\n"
		 << "  --addr-list <文件>           输出地址清单（自动启用 --addr）\n"
		 << "  --resolve-addrs <文件>       把地址清单解析成函数名，需配合 --symfs <未 strip 的根目录>\n"
		 << "  示例：板子上  perf_analyzer out.perf --addr-list addrs.txt > report.txt\n"
		 << "        主机上  perf_analyzer --resolve-addrs addrs.txt --symfs <rootfs_partition_unstripped>\n";
}

int parseOptions(int argc, char** argv, Options& opt)
{
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
		else if (a == "--no-elf")
			opt.elf = false;
		else if (a == "--symfs")
			opt.symfs = next();
		else if (a == "--maps")
			opt.mapsFile = next();
		else if (a == "--addr")
			opt.addrMode = true;
		else if (a == "--addr-list")
		{
			opt.addrList = next();
			opt.addrMode = true;
		}
		else if (a == "--resolve-addrs")
			opt.resolveAddrs = next();
		else if (a == "-h" || a == "--help")
		{
			usage(argv[0]);
			return -1;
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
	if (opt.input.empty() && opt.resolveAddrs.empty())
	{
		usage(argv[0]);
		return 1;
	}
	return 0;
}
