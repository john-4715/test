// options.cpp
#include "options.h"

#include <cstdlib>
#include <iostream>

using namespace std;

void usage(const char* prog)
{
	cerr << "Usage: " << prog << " <perf script output or folded stack file> [options]\n"
		 << "  --format auto|script|folded  input format (default: auto-detect)\n"
		 << "  --duration <sec>             capture duration, used to compute average CPU usage\n"
		 << "  --freq <Hz>                  sampling frequency (used to estimate CPU time when period is unavailable)\n"
		 << "  --top <N>                    number of entries in top lists (default 15)\n"
		 << "  --min-pct <percent>          rule check threshold (default 1.0)\n"
		 << "  --comm <thread>              analyze only the given thread\n"
		 << "  --keep-root-anomalies        keep [perf-N.map]/[JIT]/[unknown] frames at the stack bottom\n"
		 << "  --folded-out <file>          write folded stacks (can be fed directly to flamegraph.pl)\n"
		 << "  --mark-kernel                append _[k] to kernel frames in folded stacks\n";
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
		else if (a == "-h" || a == "--help")
		{
			usage(argv[0]);
			return -1;
		}
		else if (!a.empty() && a[0] == '-')
		{
			cerr << "Unknown option: " << a << "\n";
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
	return 0;
}
