// string_utils.cpp
#include "string_utils.h"

#include <iomanip>
#include <sstream>

using namespace std;

vector<string> splitWs(const string& s)
{
	vector<string> out;
	istringstream is(s);
	string t;
	while (is >> t)
		out.push_back(t);
	return out;
}

string baseName(const string& path)
{
	size_t p = path.find_last_of('/');
	return p == string::npos ? path : path.substr(p + 1);
}

string stripVersion(const string& s)
{
	size_t p = s.find('@');
	if (p != string::npos && p > 0)
		return s.substr(0, p);
	return s;
}

string pretty(const string& name, size_t maxLen)
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

string pctStr(double v, double total)
{
	ostringstream os;
	os << fixed << setprecision(1) << (total > 0 ? 100.0 * v / total : 0.0) << "%";
	return os.str();
}

string numStr(double v, int prec)
{
	ostringstream os;
	os << fixed << setprecision(prec) << v;
	return os.str();
}

bool isNumTok(const string& t)
{
	if (t.empty())
		return false;
	for (char c : t)
		if (!isdigit((unsigned char)c))
			return false;
	return true;
}
