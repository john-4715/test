// input_parser.h - parse perf script output and folded stacks
#pragma once

#include <istream>
#include <string>

#include "dataset.h"
#include "options.h"

class InputParser
{
  public:
	explicit InputParser(const Options& opt) : opt_(opt) {}

	// Auto-detect the format, returns "script" or "folded"; rewinds the stream to the beginning afterwards
	static std::string detectFormat(std::istream& in);

	// Parse using the given format; returns true if any samples were parsed
	bool parse(std::istream& in, const std::string& format, Dataset& ds) const;

	bool parsePerfScript(std::istream& in, Dataset& ds) const;
	bool parseFolded(std::istream& in, Dataset& ds) const;

  private:
	const Options& opt_;

	void addStack(Dataset& ds, const std::string& comm, std::vector<Frame>& leafFirst, double w) const;
	static Frame parseFrameLine(const std::string& line);
	static bool isTimestampTok(const std::string& t);
	static bool isPidTok(const std::string& t);
};
