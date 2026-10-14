// options.h - 命令行选项
#pragma once

#include <string>

struct Options
{
	std::string input;
	std::string format = "auto"; // auto | script | folded
	double duration = 0;		 // 采集时长（秒），用于计算平均 CPU 占用
	double freq = 0;			 // 采样频率（Hz），没有 period 信息时用于估算 CPU 时间
	int top = 15;				 // 排行榜条数
	double minPct = 1.0;		 // 规则检测的最小占比阈值（%）
	std::string foldedOut;		 // 输出折叠栈文件
	bool keepRootAnomalies = false;
	bool markKernel = false;  // 输出折叠栈时给内核帧加 _[k]
	std::string comm;		  // 只分析某个线程名
	bool elf = true;		  // perf 未解析的用户态地址，由本工具读取 ELF 符号表解析
	std::string symfs;		  // ELF 文件路径前缀（类似 perf 的 --symfs），板子上解析时留空
	std::string mapsFile;	  // /proc/PID/maps 格式的内存映射文件（perf script 未带 mmap 事件时使用）
	bool addrMode = false;	  // 无法解析函数名时显示 "文件名+0x链接地址"
	std::string addrList;	  // 输出地址清单文件（配合 --addr）
	std::string resolveAddrs; // 把地址清单解析成函数名（主机上使用，配合 --symfs）
};

// 打印用法
void usage(const char* prog);

// 解析命令行。返回值：0 继续执行；>0 应以该值作为退出码结束（含 --help 时返回 -1 表示正常退出）
int parseOptions(int argc, char** argv, Options& opt);
