// addr_resolver.cpp
#include "addr_resolver.h"
#include "elf_symbolizer.h"
#include "string_utils.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

using namespace std;

int resolveAddrList(const Options& opt)
{
	ifstream in(opt.resolveAddrs);
	if (!in)
	{
		cerr << "无法打开地址清单: " << opt.resolveAddrs << "\n";
		return 1;
	}
	if (opt.symfs.empty())
		cerr << "提示：没有指定 --symfs，将按清单中的原始路径读取文件\n";

	struct Row
	{
		string file, addr, func;
		uint64_t off = 0;
		double self = 0, incl = 0;
		bool ok = false;
	};
	vector<Row> rows;
	double total = 0;
	Symbolizer sym(opt.symfs);
	string line;
	while (getline(in, line))
	{
		if (line.empty())
			continue;
		if (line[0] == '#')
		{
			size_t p = line.find("总样本数");
			if (p != string::npos)
				total = atof(line.substr(p + string("总样本数").size()).c_str());
			continue;
		}
		istringstream is(line);
		Row r;
		if (!getline(is, r.file, '\t') || !getline(is, r.addr, '\t'))
			continue;
		is >> r.self >> r.incl;
		uint64_t vaddr = 0;
		try
		{
			vaddr = stoull(r.addr, nullptr, 16);
		}
		catch (...)
		{
			continue;
		}
		r.ok = sym.lookupFile(r.file, vaddr, r.func, r.off);
		rows.push_back(r);
	}
	if (rows.empty())
	{
		cerr << "地址清单中没有有效记录\n";
		return 1;
	}
	if (total <= 0)
		for (auto& r : rows)
			total = max(total, r.incl);

	size_t okCount = 0;
	for (auto& r : rows)
		okCount += r.ok;

	cout << "# 地址清单解析结果\n\n";
	cout << "- 地址清单：" << opt.resolveAddrs << "\n";
	cout << "- 符号目录：" << (opt.symfs.empty() ? "（未指定）" : opt.symfs) << "\n";
	cout << "- 解析成功 " << okCount << " / " << rows.size() << " 个地址，总样本数 " << numStr(total) << "\n";
	auto bad = sym.filesUnreadable();
	if (!bad.empty())
	{
		cout << "- 无法打开的文件（检查 --symfs 是否正确）：";
		for (size_t i = 0; i < bad.size(); i++)
			cout << (i ? "、" : "") << bad[i];
		cout << "\n";
	}
	auto dyn = sym.filesOnlyDynsym();
	if (!dyn.empty())
	{
		cout << "- 只有动态符号表的文件（内部函数可能解析成相邻的导出函数，需用未 strip 的版本）：";
		for (size_t i = 0; i < dyn.size(); i++)
			cout << (i ? "、" : "") << dyn[i];
		cout << "\n";
	}
	cout << "\n";

	// 按函数汇总
	// 按"文件 + 函数"汇总，不同文件中的同名函数分开统计
	struct Agg
	{
		string func, file;
		double self = 0, incl = 0;
		int addrs = 0;
	};
	map<string, Agg> byFunc;
	for (auto& r : rows)
	{
		string func = r.ok ? r.func : r.addr + "（未解析）";
		Agg& a = byFunc[r.file + '\t' + func];
		a.func = func;
		a.file = baseName(r.file);
		a.self += r.self;
		a.incl += r.incl;
		a.addrs++;
	}
	vector<pair<string, Agg>> funcs(byFunc.begin(), byFunc.end());
	sort(funcs.begin(), funcs.end(),
		[](auto& a, auto& b)
		{
			if (a.second.self != b.second.self)
				return a.second.self > b.second.self;
			return a.second.incl > b.second.incl;
		});

	cout << "## 按函数汇总\n\n| 排名 | 函数 | 文件 | 自身 | 累计 | 地址数 |\n| --- | --- | --- | --- | --- | --- |\n";
	int rank = 0;
	for (auto& f : funcs)
	{
		if (rank >= opt.top * 2)
			break;
		cout << "| " << ++rank << " | " << pretty(f.second.func) << " | " << f.second.file << " | "
			 << pctStr(f.second.self, total) << " | " << pctStr(f.second.incl, total) << " | " << f.second.addrs
			 << " |\n";
	}
	cout << "\n累计占比是该函数各地址的累计样本之和，函数递归调用时会偏大。\n\n";

	cout << "## 按地址明细\n\n| 文件 | 地址 | 函数+偏移 | 自身 | 累计 |\n| --- | --- | --- | --- | --- |\n";
	rank = 0;
	for (auto& r : rows)
	{
		if (rank++ >= opt.top * 3)
			break;
		ostringstream fn;
		if (r.ok)
			fn << pretty(r.func, 90) << "+0x" << hex << r.off;
		else
			fn << "（未解析）";
		cout << "| " << baseName(r.file) << " | " << r.addr << " | " << fn.str() << " | " << pctStr(r.self, total)
			 << " | " << pctStr(r.incl, total) << " |\n";
	}
	cout << "\n调用者一侧的地址是返回地址（调用指令的下一条），偶尔会落到相邻函数，以自身占比高的地址为准。\n";
	return 0;
}
