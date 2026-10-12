// string_utils.h - 字符串与格式化工具
#pragma once

#include <cctype>
#include <string>
#include <vector>

inline std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

inline bool startsWith(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

inline bool endsWith(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

inline bool contains(const std::string& s, const std::string& p) {
    return s.find(p) != std::string::npos;
}

inline std::string toLower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// 按空白分割
std::vector<std::string> splitWs(const std::string& s);

// 取路径中的文件名
std::string baseName(const std::string& path);

// 去掉 glibc 版本后缀：sem_trywait@@GLIBC_2.4 -> sem_trywait
std::string stripVersion(const std::string& s);

// 显示用：缩短 C++ 长类型名
std::string pretty(const std::string& name, size_t maxLen = 110);

// v / total 的百分比字符串，保留一位小数
std::string pctStr(double v, double total);

// 数字转字符串
std::string numStr(double v, int prec = 0);

// 是否为纯数字
bool isNumTok(const std::string& t);
