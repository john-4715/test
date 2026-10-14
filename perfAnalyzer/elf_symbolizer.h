// elf_symbolizer.h - 读取 ELF 符号表，把运行时地址解析成函数名
//
// 用途：板子上的 perf 不带 libelf，perf script 只能输出 "地址 [unknown] (库路径)"。
// 本模块根据进程的内存映射（PERF_RECORD_MMAP/MMAP2 事件或 /proc/PID/maps），
// 把运行时地址换算成 ELF 文件中的链接地址，再查 .symtab（没有则用 .dynsym）得到函数名。
//
// 换算过程：
//   文件偏移 = 运行时地址 - 映射起始地址 + 映射的文件偏移(pgoff)
//   链接地址 = 文件偏移 - PT_LOAD 段的 p_offset + 该段的 p_vaddr
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ElfSymbol
{
	uint64_t addr = 0; // 链接地址（ARM Thumb 函数已清除最低位）
	uint64_t size = 0;
	std::string name; // 已 demangle 的函数名
};

// 一个 ELF 文件的符号表和加载段信息
class ElfFile
{
  public:
	// 读取文件，支持 32/64 位小端 ELF；失败返回 false
	bool load(const std::string& path);

	// 文件偏移 -> 链接地址
	bool offsetToVaddr(uint64_t off, uint64_t& vaddr) const;

	// 按链接地址查找函数，找不到返回 nullptr
	const ElfSymbol* lookup(uint64_t vaddr) const;

	bool isExecutable() const { return type_ == 2; } // ET_EXEC：非 PIE 程序，运行地址即链接地址
	bool fromSymtab() const { return fromSymtab_; }	 // false 表示只有动态符号表（被 strip 过）
	size_t symbolCount() const { return syms_.size(); }

  private:
	struct Segment
	{
		uint64_t offset, filesz, vaddr;
	};
	std::vector<Segment> loads_;
	std::vector<ElfSymbol> syms_; // 按地址排序
	uint16_t type_ = 0;
	bool fromSymtab_ = false;
};

// 一条可执行内存映射
struct Mapping
{
	uint64_t start = 0, end = 0, pgoff = 0;
	int pid = -1;
	std::string path;
};

// 地址解析器：维护内存映射和 ELF 文件缓存
class Symbolizer
{
  public:
	// root：文件路径前缀，类似 perf 的 --symfs；在板子上直接解析时为空
	explicit Symbolizer(std::string root = "") : root_(std::move(root)) {}

	// 解析 perf script 中的 PERF_RECORD_MMAP / MMAP2 行，是映射行返回 true
	bool parseMmapLine(const std::string& line);

	// 读取 /proc/PID/maps 格式的文件
	bool loadMapsFile(const std::string& file, int pid = -1);

	// 解析地址：dso 为 perf script 中括号里的库路径，pid 未知时传 -1
	bool resolve(uint64_t ip, const std::string& dso, int pid, std::string& name);

	// 只做地址换算：运行时地址 -> 文件内链接地址（不需要符号表）
	bool toVaddr(uint64_t ip, const std::string& dso, int pid, uint64_t& vaddr);

	// 在指定文件中按链接地址查函数名，offset 为相对函数起始的偏移
	bool lookupFile(const std::string& path, uint64_t vaddr, std::string& name, uint64_t& offset);

	size_t mappingCount() const { return mappingCount_; }
	size_t resolvedFrames() const { return resolved_; }
	size_t failedFrames() const { return failed_; }

	// 解析过、但只有动态符号表或打不开的文件（用于报告提示）
	std::vector<std::string> filesOnlyDynsym() const;
	std::vector<std::string> filesUnreadable() const;

  private:
	std::string root_;
	std::unordered_map<std::string, std::vector<Mapping>> maps_;	  // 按库路径索引
	std::unordered_map<std::string, std::unique_ptr<ElfFile>> cache_; // nullptr 表示读取失败
	size_t mappingCount_ = 0, resolved_ = 0, failed_ = 0;

	void addMapping(const Mapping& m);
	ElfFile* getElf(const std::string& path);
};
