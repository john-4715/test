// symbol_rules.h - function name classification rules (kernel/library/application, syscall entries, anomaly frames, etc.)
#pragma once

#include <string>
#include <unordered_set>

// Common libc/system library function names
extern const std::unordered_set<std::string> kLibcNames;

// Functions that "should never be hot", commonly seen when the libc version does not match
extern const std::unordered_set<std::string> kSuspiciousLibc;

// Syscall entry points (used to decide whether a sample is inside a syscall)
bool isSyscallEntry(const std::string& n);

// Kernel functions (heuristic when folded stacks have no _[k] marker)
bool looksLikeKernel(const std::string& n);

// Anomaly frames at the stack bottom: [perf-PID.map], [JIT], [unknown]
bool isRootAnomaly(const std::string& n);

// Unresolved frames: library name only or [unknown]
bool isUnresolved(const std::string& n);

// Library / runtime functions (not application code)
bool isLibName(const std::string& raw);
