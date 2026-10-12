// input_parser.cpp
#include "input_parser.h"
#include "string_utils.h"
#include "symbol_rules.h"

#include <cmath>
#include <string>
#include <vector>

using namespace std;

bool InputParser::isTimestampTok(const string& t) {
    if (t.size() < 2 || t.back() != ':') return false;
    bool dot = false;
    for (size_t i = 0; i + 1 < t.size(); i++) {
        if (t[i] == '.') dot = true;
        else if (!isdigit((unsigned char)t[i])) return false;
    }
    return dot;
}

bool InputParser::isPidTok(const string& t) {
    if (t.empty()) return false;
    for (char c : t)
        if (!isdigit((unsigned char)c) && c != '/') return false;
    return isdigit((unsigned char)t[0]);
}

Frame InputParser::parseFrameLine(const string& line) {
    string t = trim(line);
    size_t sp = t.find_first_of(" \t");
    string rest = sp == string::npos ? "" : trim(t.substr(sp + 1));
    string dso;
    if (!rest.empty() && rest.back() == ')') {
        size_t p = rest.rfind(" (");
        if (p != string::npos) {
            dso = rest.substr(p + 2, rest.size() - p - 3);
            rest = trim(rest.substr(0, p));
        } else if (rest[0] == '(') {
            dso = rest.substr(1, rest.size() - 2);
            rest.clear();
        }
    }
    size_t plus = rest.rfind("+0x");
    if (plus != string::npos && plus > 0) rest = rest.substr(0, plus);
    rest = stripVersion(rest);

    bool kernel = contains(dso, "kernel.kallsyms") || startsWith(dso, "[kernel") ||
                  endsWith(dso, ".ko") || (startsWith(dso, "[") && endsWith(dso, ".ko]"));
    string name = rest;
    if (name.empty() || name == "[unknown]") {
        if (!dso.empty() && dso != "[unknown]") {
            string b = baseName(dso);
            if (!b.empty() && b[0] == '[') name = b;
            else name = "[" + b + "]";
        } else {
            name = "[unknown]";
        }
    }
    return {name, kernel};
}

void InputParser::addStack(Dataset& ds, const string& comm, vector<Frame>& leafFirst,
                           double w) const {
    if (!opt_.comm.empty() && comm != opt_.comm) return;
    Stack st;
    st.w = w;
    st.f.push_back(ds.names.id(comm.empty() ? "[unknown-comm]" : comm));
    st.k.push_back(0);
    for (auto it = leafFirst.rbegin(); it != leafFirst.rend(); ++it) {
        st.f.push_back(ds.names.id(it->name));
        st.k.push_back(it->kernel ? 1 : 0);
    }
    if (!opt_.keepRootAnomalies) {
        bool stripped = false;
        while (st.f.size() > 2 && isRootAnomaly(ds.names.str(st.f[1]))) {
            st.f.erase(st.f.begin() + 1);
            st.k.erase(st.k.begin() + 1);
            stripped = true;
        }
        if (stripped) ds.anomalyStripped += w;
    }
    ds.totalSamples += w;
    ds.stacks.push_back(std::move(st));
}

bool InputParser::parsePerfScript(istream& in, Dataset& ds) const {
    string line, comm;
    vector<Frame> frames;
    bool inSample = false;
    double period = 0;
    auto flush = [&]() {
        if (inSample) {
            addStack(ds, comm, frames, 1.0);
            ds.periodSum += period;
        }
        frames.clear();
        inSample = false;
        period = 0;
    };
    while (getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (trim(line).empty()) {
            flush();
            continue;
        }
        if (line[0] == '#') continue;
        if (line[0] == ' ' || line[0] == '\t') {
            if (inSample) frames.push_back(parseFrameLine(line));
            continue;
        }
        // 样本头：comm pid[/tid] [cpu] time: [period] event:
        flush();
        vector<string> tok = splitWs(line);
        int ts = -1;
        for (int i = 0; i < (int)tok.size(); i++)
            if (isTimestampTok(tok[i])) { ts = i; break; }
        if (ts < 1) continue;
        int pidIdx = ts - 1;
        if (pidIdx >= 0 && tok[pidIdx].size() > 2 && tok[pidIdx][0] == '[') pidIdx--;
        if (pidIdx < 0 || !isPidTok(tok[pidIdx])) continue;
        comm.clear();
        for (int i = 0; i < pidIdx; i++) {
            if (i) comm += ' ';
            comm += tok[i];
        }
        int ev = ts + 1;
        if (ev < (int)tok.size() && isNumTok(tok[ev])) {
            period = stod(tok[ev]);
            ev++;
        }
        if (ev < (int)tok.size() && ds.event.empty()) {
            string e = tok[ev];
            if (!e.empty() && e.back() == ':') e.pop_back();
            ds.event = e;
        }
        inSample = true;
    }
    flush();
    if (contains(ds.event, "cpu-clock") || contains(ds.event, "task-clock")) ds.periodIsNs = true;
    ds.inputFormat = "perf script";
    return !ds.stacks.empty();
}

bool InputParser::parseFolded(istream& in, Dataset& ds) const {
    struct Raw { vector<Frame> leafFirst; string comm; double cnt; };
    vector<Raw> raws;
    string line;
    while (getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t sp = line.find_last_of(' ');
        if (sp == string::npos) continue;
        double cnt = 0;
        try { cnt = stod(line.substr(sp + 1)); } catch (...) { continue; }
        string path = line.substr(0, sp);
        vector<string> parts;
        size_t start = 0;
        while (true) {
            size_t p = path.find(';', start);
            parts.push_back(path.substr(start, p == string::npos ? string::npos : p - start));
            if (p == string::npos) break;
            start = p + 1;
        }
        if (parts.empty()) continue;
        Raw r;
        r.comm = parts[0];
        r.cnt = cnt;
        for (size_t i = parts.size(); i-- > 1;) {
            string n = parts[i];
            bool k = false;
            if (endsWith(n, "_[k]")) { n = n.substr(0, n.size() - 4); k = true; }
            else if (endsWith(n, "_[j]") || endsWith(n, "_[i]") || endsWith(n, "_[w]"))
                n = n.substr(0, n.size() - 4);
            else k = looksLikeKernel(n);
            r.leafFirst.push_back({stripVersion(n), k});
        }
        raws.push_back(std::move(r));
    }
    if (raws.empty()) return false;

    // 自动识别计数单位：stackcollapse 新版本会累加 period（cpu-clock 为纳秒）
    double mn = 1e300;
    for (auto& r : raws) if (r.cnt > 0) mn = min(mn, r.cnt);
    double unit = 1;
    if (mn >= 100000) {
        bool allMultiple = true;
        for (auto& r : raws) {
            double q = r.cnt / mn;
            if (fabs(q - llround(q)) > 0.01) { allMultiple = false; break; }
        }
        if (allMultiple) unit = mn;
    }
    for (auto& r : raws) {
        addStack(ds, r.comm, r.leafFirst, r.cnt / unit);
        if (unit > 1) ds.periodSum += r.cnt;
    }
    if (unit > 1) {
        ds.periodIsNs = true;
        ds.event = "cpu-clock（由计数单位推断，每样本 " + numStr(unit) + " ns）";
    }
    ds.inputFormat = "折叠栈";
    return !ds.stacks.empty();
}

string InputParser::detectFormat(istream& in) {
    string line;
    int folded = 0, script = 0, checked = 0;
    while (checked < 200 && getline(in, line)) {
        if (trim(line).empty() || line[0] == '#') continue;
        checked++;
        if (line[0] == ' ' || line[0] == '\t') script++;
        else {
            string t = trim(line);
            size_t sp = t.find_last_of(' ');
            if (sp != string::npos && t.find(';') != string::npos && isNumTok(t.substr(sp + 1))) folded++;
        }
    }
    in.clear();
    in.seekg(0);
    return (folded > script) ? "folded" : "script";
}

bool InputParser::parse(istream& in, const string& format, Dataset& ds) const {
    return format == "folded" ? parseFolded(in, ds) : parsePerfScript(in, ds);
}
