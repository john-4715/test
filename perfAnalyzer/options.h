// options.h - 命令行选项
#pragma once

#include <string>

struct Options {
    std::string input;
    std::string format = "auto";  // auto | script | folded
    double duration = 0;          // 采集时长（秒），用于计算平均 CPU 占用
    double freq = 0;              // 采样频率（Hz），没有 period 信息时用于估算 CPU 时间
    int top = 15;                 // 排行榜条数
    double minPct = 1.0;          // 规则检测的最小占比阈值（%）
    std::string foldedOut;        // 输出折叠栈文件
    bool keepRootAnomalies = false;
    bool markKernel = false;      // 输出折叠栈时给内核帧加 _[k]
    std::string comm;             // 只分析某个线程名
};

// 打印用法
void usage(const char* prog);

// 解析命令行。返回值：0 继续执行；>0 应以该值作为退出码结束（含 --help 时返回 -1 表示正常退出）
int parseOptions(int argc, char** argv, Options& opt);
