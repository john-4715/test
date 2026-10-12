// main.cpp - perf_analyzer entry point
//
// Analyzes perf capture data and automatically reports data quality checks, thread distribution, hotspot rankings, rule checks and optimization priorities.
// Input: perf script text output or stackcollapse folded stacks (auto-detected).
//
// Usage: ./perf_analyzer out.perf --duration 60
//       ./perf_analyzer out.folded --duration 60 --top 20
//       ./perf_analyzer out.perf --folded-out out.folded

#include <fstream>
#include <iostream>

#include "analyzer.h"
#include "dataset.h"
#include "input_parser.h"
#include "options.h"

using namespace std;

int main(int argc, char** argv)
{
	Options opt;
	int rc = parseOptions(argc, argv, opt);
	if (rc < 0)
		return 0; // --help
	if (rc > 0)
		return rc;

	ifstream in(opt.input);
	if (!in)
	{
		cerr << "Cannot open file: " << opt.input << "\n";
		return 1;
	}

	string fmt = opt.format == "auto" ? InputParser::detectFormat(in) : opt.format;

	Dataset ds;
	InputParser parser(opt);
	if (!parser.parse(in, fmt, ds))
	{
		cerr << "No samples were parsed. Please check that the input is perf script output or a folded stack file";
		if (!opt.comm.empty())
			cerr << ", and that the thread name " << opt.comm << " is correct";
		cerr << ".\n";
		return 1;
	}

	if (!opt.foldedOut.empty())
		ds.writeFolded(opt.foldedOut, opt.markKernel);

	Analyzer an(ds, opt);
	an.run();
	return 0;
}
