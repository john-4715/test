// main.cpp - perf_analyzer 入口
//
// 分析 perf 采集数据，自动输出数据质量检查、线程分布、热点排行、规则检测和优化优先级。
// 输入：perf script 文本输出或 stackcollapse 折叠栈（自动识别）。
//
// 用法：./perf_analyzer out.perf --duration 60
//       ./perf_analyzer out.folded --duration 60 --top 20
//       ./perf_analyzer out.perf --folded-out out.folded

#include <fstream>
#include <iostream>

#include "addr_resolver.h"
#include "analyzer.h"
#include "dataset.h"
#include "elf_symbolizer.h"
#include "input_parser.h"
#include "options.h"

using namespace std;

int main(int argc, char** argv)
{
	Options opt;
	int rc = parseOptions(argc, argv, opt);
	if (rc < 0)
		return 0; // --help
	if (rc > 0)
		return rc;

	// 地址清单换算模式：不需要输入文件
	if (!opt.resolveAddrs.empty())
		return resolveAddrList(opt);

	ifstream in(opt.input);
	if (!in)
	{
		cerr << "无法打开文件: " << opt.input << "\n";
		return 1;
	}

	string fmt = opt.format == "auto" ? InputParser::detectFormat(in) : opt.format;

	Dataset ds;
	Symbolizer sym(opt.symfs);
	if (opt.elf && !opt.mapsFile.empty() && !sym.loadMapsFile(opt.mapsFile))
		cerr << "警告：无法读取内存映射文件 " << opt.mapsFile << "\n";
	InputParser parser(opt, opt.elf ? &sym : nullptr);
	if (!parser.parse(in, fmt, ds))
	{
		cerr << "没有解析到任何样本。请确认输入是 perf script 输出或折叠栈文件";
		if (!opt.comm.empty())
			cerr << "，以及线程名 " << opt.comm << " 是否正确";
		cerr << "。\n";
		return 1;
	}

	if (opt.elf)
	{
		ds.elf.enabled = true;
		ds.elf.mappings = sym.mappingCount();
		ds.elf.resolved = sym.resolvedFrames();
		ds.elf.failed = sym.failedFrames();
		ds.elf.onlyDynsym = sym.filesOnlyDynsym();
		ds.elf.unreadable = sym.filesUnreadable();
	}

	if (!opt.foldedOut.empty())
		ds.writeFolded(opt.foldedOut, opt.markKernel);

	Analyzer an(ds, opt);
	an.run();
	if (!opt.addrList.empty() && !an.writeAddrList(opt.addrList))
		cerr << "无法写入地址清单: " << opt.addrList << "\n";
	return 0;
}
