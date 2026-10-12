// main.cpp - perf_analyzer 入口
//
// 分析 perf 采集数据，自动输出数据质量检查、线程分布、热点排行、规则检测和优化优先级。
// 输入：perf script 文本输出或 stackcollapse 折叠栈（自动识别）。
//
// 用法：./perf_analyzer out.perf --duration 60
//       ./perf_analyzer out.folded --duration 60 --top 20
//       ./perf_analyzer out.perf --folded-out out.folded

#include <fstream>
#include <iostream>

#include "analyzer.h"
#include "dataset.h"
#include "input_parser.h"
#include "options.h"

using namespace std;

int main(int argc, char** argv) {
    Options opt;
    int rc = parseOptions(argc, argv, opt);
    if (rc < 0) return 0;   // --help
    if (rc > 0) return rc;

    ifstream in(opt.input);
    if (!in) { cerr << "无法打开文件: " << opt.input << "\n"; return 1; }

    string fmt = opt.format == "auto" ? InputParser::detectFormat(in) : opt.format;

    Dataset ds;
    InputParser parser(opt);
    if (!parser.parse(in, fmt, ds)) {
        cerr << "没有解析到任何样本。请确认输入是 perf script 输出或折叠栈文件";
        if (!opt.comm.empty()) cerr << "，以及线程名 " << opt.comm << " 是否正确";
        cerr << "。\n";
        return 1;
    }

    if (!opt.foldedOut.empty()) ds.writeFolded(opt.foldedOut, opt.markKernel);

    Analyzer an(ds, opt);
    an.run();
    return 0;
}
