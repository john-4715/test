// analyzer.h - 统计分析与报告输出
#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "dataset.h"
#include "options.h"

// 一条规则检测结果
struct Finding {
    std::string title;
    double weight = 0;          // 涉及样本数
    std::vector<std::string> details;
    std::string advice;
    std::string effort;              // 改动难度
};

class Analyzer {
public:
    Analyzer(Dataset& d, const Options& o) : ds(d), opt(o), N(ds.names.size()) {}

    void run();

    // 输出地址清单：文件路径、链接地址、自身样本数、累计样本数（--addr-list）
    bool writeAddrList(const std::string& path) const;

private:
    Dataset& ds;
    const Options& opt;
    size_t N;
    double W = 0;

    std::vector<double> selfW, inclW;
    std::map<std::string, double> threadW;
    double kernelLeafW = 0, syscallW = 0;
    std::map<std::string, double> syscallWrapperW;   // 用户态系统调用包装函数 -> 样本
    std::vector<Finding> findings;
    std::string kernelArch;                 // 由内核函数名推断的内核架构
    std::set<int> entryIds;                 // 线程入口包装函数（多数线程栈底都出现的业务函数）
    bool isEntry(int id) const { return entryIds.count(id) > 0; }
    void detectEntryFrames();

    const std::string& nm(int id) const;
    bool isApp(const Stack& s, size_t i) const;

    // ---- 聚合 ----
    void aggregate();

    // 包含任一目标函数的样本数（每个样本只计一次）
    double inclOf(const std::set<std::string>& targets, const std::string& thread = "") const;

    // 目标函数最近的业务调用者
    std::vector<std::pair<std::string, double>> appCallers(const std::set<std::string>& targets, size_t n = 5) const;

    // 各线程中目标函数的占比
    std::vector<std::pair<std::string, double>> byThread(const std::set<std::string>& targets) const;

    // ---- 输出 ----
    void printHeader() const;

    void printQuality() const;

    void printOverview() const;

    void printThreads() const;

    // 沿累计占比最大的子节点向下走，打印线程内的主干调用路径
    void printHotPath(const std::string& thread, double threadWeight) const;

    void printTopLists() const;

    bool isKernelId(int id) const;

    std::string kindOf(int id) const;

    // ---- 规则检测 ----
    void addCallerDetails(Finding& f, const std::set<std::string>& targets) const;

    bool over(double w) const;

    void detect();

    void printFindings() const;

    void printCaveats() const;
};
