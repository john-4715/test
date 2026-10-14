// dataset.h - 数据模型：函数名字符串池、调用栈、数据集
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// 函数名字符串池：把字符串映射为整数 id，减少内存和比较开销
class Interner
{
  public:
	int id(const std::string& s);
	const std::string& str(int i) const { return v[i]; }
	size_t size() const { return v.size(); }
	// 查找已有的 id，不存在返回 -1
	int find(const std::string& s) const;

  private:
	std::unordered_map<std::string, int> m;
	std::vector<std::string> v;
};

// 一个调用栈样本
struct Stack
{
	std::vector<int> f;	 // 从根到叶：f[0] 为线程名，最后一个为采样点所在函数
	std::vector<char> k; // 对应帧是否为内核帧
	double w = 0;		 // 样本数
};

// 一个解析后的栈帧
struct Frame
{
	std::string name;
	bool kernel = false;
	uint64_t ip = 0; // 运行时地址
	std::string dso; // 所属文件路径（perf script 括号中的内容）
};

// ELF 符号解析统计（用于报告中的数据质量检查）
struct ElfStats
{
	bool enabled = false;
	size_t mappings = 0;				 // 读取到的可执行内存映射数
	size_t resolved = 0;				 // 由 perf_analyzer 解析出函数名的栈帧数
	size_t failed = 0;					 // 尝试解析但失败的栈帧数
	std::vector<std::string> onlyDynsym; // 只有动态符号表（被 strip）的文件
	std::vector<std::string> unreadable; // 打不开或不是 ELF 的文件
	size_t addrFrames = 0;				 // --addr 模式下以地址形式输出的栈帧数
};

// 解析后的完整数据集
struct Dataset
{
	Interner names;
	std::vector<Stack> stacks;
	double totalSamples = 0;
	double periodSum = 0; // period 累加（cpu-clock 时单位为 ns）
	std::string event;	  // 采样事件名
	bool periodIsNs = false;
	double anomalyStripped = 0; // 去掉栈底异常帧的样本数
	std::string inputFormat;
	ElfStats elf;
	std::unordered_map<std::string, std::string> addrDso; // 地址帧名 -> 板子上的完整文件路径

	// 输出折叠栈（可直接交给 flamegraph.pl）
	void writeFolded(const std::string& path, bool markKernel) const;
};
