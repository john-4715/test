// symbol_rules.h - 函数名分类规则（内核/库/业务、系统调用入口、异常帧等）
#pragma once

#include <string>
#include <unordered_set>

// 常见 libc/系统库函数名
extern const std::unordered_set<std::string> kLibcNames;

// libc 版本不匹配时常见的"不可能出现在热点里"的函数名
extern const std::unordered_set<std::string> kSuspiciousLibc;

// 系统调用入口（用于判断一个样本是否处于系统调用中）
bool isSyscallEntry(const std::string& n);

// 内核函数（折叠栈没有 _[k] 标记时的启发式判断）
bool looksLikeKernel(const std::string& n);

// 栈底异常帧：[perf-PID.map]、[JIT]、[unknown]
bool isRootAnomaly(const std::string& n);

// 未解析帧：只有库名或 [unknown]
bool isUnresolved(const std::string& n);

// 库函数 / 运行时函数（非业务代码）
bool isLibName(const std::string& raw);
