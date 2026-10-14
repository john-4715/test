// dataset.cpp
#include "dataset.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>

using namespace std;

int Interner::id(const string& s)
{
	auto it = m.find(s);
	if (it != m.end())
		return it->second;
	int i = (int)v.size();
	v.push_back(s);
	m.emplace(s, i);
	return i;
}

int Interner::find(const string& s) const
{
	auto it = m.find(s);
	return it == m.end() ? -1 : it->second;
}

void Dataset::writeFolded(const string& path, bool markKernel) const
{
	map<string, double> agg;
	for (const auto& s : stacks)
	{
		string line;
		for (size_t i = 0; i < s.f.size(); i++)
		{
			if (i)
				line += ';';
			line += names.str(s.f[i]);
			if (markKernel && s.k[i])
				line += "_[k]";
		}
		agg[line] += s.w;
	}
	ofstream out(path);
	for (auto& p : agg)
		out << p.first << ' ' << llround(p.second) << '\n';
	cerr << "已输出折叠栈：" << path << "（" << agg.size() << " 条）\n";
}
