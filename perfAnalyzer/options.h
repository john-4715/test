// options.h - command-line options
#pragma once

#include <string>

struct Options {
    std::string input;
    std::string format = "auto";  // auto | script | folded
    double duration = 0;          // capture duration (seconds), used to compute average CPU usage
    double freq = 0;              // sampling frequency (Hz), used to estimate CPU time when period is unavailable
    int top = 15;                 // number of entries in top lists
    double minPct = 1.0;          // minimum share threshold for rule checks (%)
    std::string foldedOut;        // output folded stack file
    bool keepRootAnomalies = false;
    bool markKernel = false;      // append _[k] to kernel frames in folded output
    std::string comm;             // analyze only this thread name
};

// Print usage
void usage(const char* prog);

// Parse the command line. Returns 0 to continue; >0 means exit with that code (returns -1 for --help, meaning normal exit)
int parseOptions(int argc, char** argv, Options& opt);
