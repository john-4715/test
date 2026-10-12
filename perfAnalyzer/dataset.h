// dataset.h - data model: function name string pool, call stacks, dataset
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Function name string pool: maps strings to integer ids to reduce memory and comparison cost
class Interner
{
  public:
	int id(const std::string& s);
	const std::string& str(int i) const { return v[i]; }
	size_t size() const { return v.size(); }
	// Look up an existing id; returns -1 if not found
	int find(const std::string& s) const;

  private:
	std::unordered_map<std::string, int> m;
	std::vector<std::string> v;
};

// One call stack sample
struct Stack
{
	std::vector<int> f;	 // root to leaf: f[0] is the thread name, the last one is the sampled function
	std::vector<char> k; // whether the corresponding frame is a kernel frame
	double w = 0;		 // sample count
};

// One parsed stack frame
struct Frame
{
	std::string name;
	bool kernel;
};

// Complete parsed dataset
struct Dataset
{
	Interner names;
	std::vector<Stack> stacks;
	double totalSamples = 0;
	double periodSum = 0; // accumulated period (in ns for cpu-clock)
	std::string event;	  // sampling event name
	bool periodIsNs = false;
	double anomalyStripped = 0; // samples whose bottom anomaly frames were stripped
	std::string inputFormat;

	// Write folded stacks (can be fed directly to flamegraph.pl)
	void writeFolded(const std::string& path, bool markKernel) const;
};
