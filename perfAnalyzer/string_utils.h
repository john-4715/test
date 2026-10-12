// string_utils.h - string and formatting utilities
#pragma once

#include <cctype>
#include <string>
#include <vector>

inline std::string trim(const std::string& s)
{
	size_t b = s.find_first_not_of(" \t\r\n");
	if (b == std::string::npos)
		return "";
	size_t e = s.find_last_not_of(" \t\r\n");
	return s.substr(b, e - b + 1);
}

inline bool startsWith(const std::string& s, const std::string& p)
{
	return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

inline bool endsWith(const std::string& s, const std::string& p)
{
	return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

inline bool contains(const std::string& s, const std::string& p) { return s.find(p) != std::string::npos; }

inline std::string toLower(std::string s)
{
	for (auto& c : s)
		c = (char)std::tolower((unsigned char)c);
	return s;
}

// Split on whitespace
std::vector<std::string> splitWs(const std::string& s);

// Get the file name from a path
std::string baseName(const std::string& path);

// Strip the glibc version suffix: sem_trywait@@GLIBC_2.4 -> sem_trywait
std::string stripVersion(const std::string& s);

// For display: shorten long C++ type names
std::string pretty(const std::string& name, size_t maxLen = 110);

// Percentage string of v / total, with one decimal place
std::string pctStr(double v, double total);

// Number to string
std::string numStr(double v, int prec = 0);

// Whether the string is all digits
bool isNumTok(const std::string& t);
