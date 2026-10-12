// analyzer.h - statistical analysis and report output
#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "dataset.h"
#include "options.h"

// Result of one rule check
struct Finding
{
	std::string title;
	double weight = 0; // number of samples involved
	std::vector<std::string> details;
	std::string advice;
	std::string effort; // effort required to fix
};

class Analyzer
{
  public:
	Analyzer(Dataset& d, const Options& o) : ds(d), opt(o), N(ds.names.size()) {}

	void run();

  private:
	Dataset& ds;
	const Options& opt;
	size_t N;
	double W = 0;

	std::vector<double> selfW, inclW;
	std::map<std::string, double> threadW;
	double kernelLeafW = 0, syscallW = 0;
	std::map<std::string, double> syscallWrapperW; // user-space syscall wrapper -> samples
	std::vector<Finding> findings;

	const std::string& nm(int id) const;
	bool isApp(const Stack& s, size_t i) const;

	// ---- Aggregation ----
	void aggregate();

	// Number of samples containing any target function (each sample counted once)
	double inclOf(const std::set<std::string>& targets, const std::string& thread = "") const;

	// Nearest application caller of the target functions
	std::vector<std::pair<std::string, double>> appCallers(const std::set<std::string>& targets, size_t n = 5) const;

	// Share of the target functions in each thread
	std::vector<std::pair<std::string, double>> byThread(const std::set<std::string>& targets) const;

	// ---- Output ----
	void printHeader() const;

	void printQuality() const;

	void printOverview() const;

	void printThreads() const;

	// Walk down the child with the largest inclusive share and print the thread's main call path
	void printHotPath(const std::string& thread, double threadWeight) const;

	void printTopLists() const;

	bool isKernelId(int id) const;

	std::string kindOf(int id) const;

	// ---- Rule checks ----
	void addCallerDetails(Finding& f, const std::set<std::string>& targets) const;

	bool over(double w) const;

	void detect();

	void printFindings() const;

	void printCaveats() const;
};
