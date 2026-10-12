// dataset.h - 数据模型：函数名字符串池、调用栈、数据集
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// 函数名字符串池：把字符串映射为整数 id，减少内存和比较开销
class Interner {
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
struct Stack {
    std::vector<int> f;   // 从根到叶：f[0] 为线程名，最后一个为采样点所在函数
    std::vector<char> k;  // 对应帧是否为内核帧
    double w = 0;         // 样本数
};

// 一个解析后的栈帧
struct Frame {
    std::string name;
    bool kernel;
};

// 解析后的完整数据集
struct Dataset {
    Interner names;
    std::vector<Stack> stacks;
    double totalSamples = 0;
    double periodSum = 0;        // period 累加（cpu-clock 时单位为 ns）
    std::string event;           // 采样事件名
    bool periodIsNs = false;
    double anomalyStripped = 0;  // 去掉栈底异常帧的样本数
    std::string inputFormat;

    // 输出折叠栈（可直接交给 flamegraph.pl）
    void writeFolded(const std::string& path, bool markKernel) const;
};
