// elf_symbolizer.cpp
#include "elf_symbolizer.h"
#include "string_utils.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <fstream>
#include <sstream>

using namespace std;

namespace
{

// ELF 常量（避免依赖系统 <elf.h>，方便交叉编译）
constexpr uint8_t ELFCLASS32 = 1;
constexpr uint8_t ELFCLASS64 = 2;
constexpr uint8_t ELFDATA2LSB = 1;
constexpr uint32_t PT_LOAD = 1;
constexpr uint32_t SHT_SYMTAB = 2;
constexpr uint32_t SHT_DYNSYM = 11;
constexpr uint8_t STT_FUNC = 2;
constexpr uint8_t STT_GNU_IFUNC = 10;
constexpr uint16_t SHN_UNDEF = 0;
constexpr uint16_t EM_ARM = 40;

template <typename T> T rd(const vector<char>& b, size_t off)
{
	T v{};
	if (off + sizeof(T) <= b.size())
		memcpy(&v, b.data() + off, sizeof(T));
	return v;
}

bool readAt(ifstream& f, uint64_t off, size_t len, vector<char>& out)
{
	out.assign(len, 0);
	f.clear();
	f.seekg((streamoff)off);
	if (!f)
		return false;
	f.read(out.data(), (streamsize)len);
	return (size_t)f.gcount() == len;
}

string demangle(const string& s)
{
	if (!startsWith(s, "_Z"))
		return s;
	int status = 0;
	char* d = abi::__cxa_demangle(s.c_str(), nullptr, nullptr, &status);
	if (status == 0 && d)
	{
		string r(d);
		free(d);
		return r;
	}
	free(d);
	return s;
}

struct SectionHdr
{
	uint32_t name = 0, type = 0, link = 0;
	uint64_t offset = 0, size = 0, entsize = 0;
};

} // namespace

// ---------------------------------------------------------------------------
// ElfFile
// ---------------------------------------------------------------------------
bool ElfFile::load(const string& path)
{
	ifstream f(path, ios::binary);
	if (!f)
		return false;

	vector<char> eh;
	if (!readAt(f, 0, 64, eh))
	{
		// 32 位 ELF 头只有 52 字节，文件很小时允许读不满 64 字节
		if (!readAt(f, 0, 52, eh))
			return false;
	}
	if (memcmp(eh.data(),
			"\x7f"
			"ELF",
			4) != 0)
		return false;
	uint8_t cls = (uint8_t)eh[4];
	if ((uint8_t)eh[5] != ELFDATA2LSB)
		return false; // 只支持小端（ARM/x86 均为小端）
	bool is64 = cls == ELFCLASS64;
	if (!is64 && cls != ELFCLASS32)
		return false;

	type_ = rd<uint16_t>(eh, 16);
	uint16_t machine = rd<uint16_t>(eh, 18);
	uint64_t phoff, shoff;
	uint16_t phentsize, phnum, shentsize, shnum;
	if (is64)
	{
		phoff = rd<uint64_t>(eh, 32);
		shoff = rd<uint64_t>(eh, 40);
		phentsize = rd<uint16_t>(eh, 54);
		phnum = rd<uint16_t>(eh, 56);
		shentsize = rd<uint16_t>(eh, 58);
		shnum = rd<uint16_t>(eh, 60);
	}
	else
	{
		phoff = rd<uint32_t>(eh, 28);
		shoff = rd<uint32_t>(eh, 32);
		phentsize = rd<uint16_t>(eh, 42);
		phnum = rd<uint16_t>(eh, 44);
		shentsize = rd<uint16_t>(eh, 46);
		shnum = rd<uint16_t>(eh, 48);
	}

	// 程序头：记录 PT_LOAD 段，用于文件偏移 -> 链接地址
	vector<char> ph;
	if (phnum && readAt(f, phoff, (size_t)phentsize * phnum, ph))
	{
		for (uint16_t i = 0; i < phnum; i++)
		{
			size_t o = (size_t)i * phentsize;
			uint32_t ptype = rd<uint32_t>(ph, o);
			if (ptype != PT_LOAD)
				continue;
			Segment s;
			if (is64)
			{
				s.offset = rd<uint64_t>(ph, o + 8);
				s.vaddr = rd<uint64_t>(ph, o + 16);
				s.filesz = rd<uint64_t>(ph, o + 32);
			}
			else
			{
				s.offset = rd<uint32_t>(ph, o + 4);
				s.vaddr = rd<uint32_t>(ph, o + 8);
				s.filesz = rd<uint32_t>(ph, o + 16);
			}
			loads_.push_back(s);
		}
	}

	// 节头：找 .symtab，没有则用 .dynsym
	vector<char> sh;
	if (!shnum || !readAt(f, shoff, (size_t)shentsize * shnum, sh))
		return !loads_.empty();
	vector<SectionHdr> secs(shnum);
	for (uint16_t i = 0; i < shnum; i++)
	{
		size_t o = (size_t)i * shentsize;
		SectionHdr& s = secs[i];
		s.name = rd<uint32_t>(sh, o);
		s.type = rd<uint32_t>(sh, o + 4);
		if (is64)
		{
			s.offset = rd<uint64_t>(sh, o + 24);
			s.size = rd<uint64_t>(sh, o + 32);
			s.link = rd<uint32_t>(sh, o + 40);
			s.entsize = rd<uint64_t>(sh, o + 56);
		}
		else
		{
			s.offset = rd<uint32_t>(sh, o + 16);
			s.size = rd<uint32_t>(sh, o + 20);
			s.link = rd<uint32_t>(sh, o + 24);
			s.entsize = rd<uint32_t>(sh, o + 36);
		}
	}
	int symIdx = -1;
	for (int i = 0; i < (int)secs.size(); i++)
		if (secs[i].type == SHT_SYMTAB)
		{
			symIdx = i;
			fromSymtab_ = true;
			break;
		}
	if (symIdx < 0)
		for (int i = 0; i < (int)secs.size(); i++)
			if (secs[i].type == SHT_DYNSYM)
			{
				symIdx = i;
				break;
			}
	if (symIdx < 0)
		return true; // 没有任何符号表，仍可用于地址换算

	const SectionHdr& st = secs[symIdx];
	if (st.link >= secs.size())
		return true;
	const SectionHdr& str = secs[st.link];
	vector<char> symData, strData;
	if (!readAt(f, st.offset, (size_t)st.size, symData))
		return true;
	if (!readAt(f, str.offset, (size_t)str.size, strData))
		return true;

	size_t ent = st.entsize ? (size_t)st.entsize : (is64 ? 24 : 16);
	size_t n = symData.size() / ent;
	syms_.reserve(n);
	for (size_t i = 0; i < n; i++)
	{
		size_t o = i * ent;
		uint32_t nameOff;
		uint64_t value, size;
		uint8_t info;
		uint16_t shndx;
		if (is64)
		{
			nameOff = rd<uint32_t>(symData, o);
			info = (uint8_t)symData[o + 4];
			shndx = rd<uint16_t>(symData, o + 6);
			value = rd<uint64_t>(symData, o + 8);
			size = rd<uint64_t>(symData, o + 16);
		}
		else
		{
			nameOff = rd<uint32_t>(symData, o);
			value = rd<uint32_t>(symData, o + 4);
			size = rd<uint32_t>(symData, o + 8);
			info = (uint8_t)symData[o + 12];
			shndx = rd<uint16_t>(symData, o + 14);
		}
		uint8_t stype = info & 0xf;
		if ((stype != STT_FUNC && stype != STT_GNU_IFUNC) || shndx == SHN_UNDEF || value == 0)
			continue;
		if (nameOff >= strData.size())
			continue;
		const char* nm = strData.data() + nameOff;
		size_t len = strnlen(nm, strData.size() - nameOff);
		if (len == 0 || nm[0] == '$')
			continue; // 跳过 ARM 映射符号 $a/$t/$d
		ElfSymbol s;
		s.addr = (machine == EM_ARM) ? (value & ~1ULL) : value; // Thumb 函数地址最低位为 1
		s.size = size;
		s.name = stripVersion(demangle(string(nm, len)));
		syms_.push_back(std::move(s));
	}
	sort(syms_.begin(), syms_.end(),
		[](const ElfSymbol& a, const ElfSymbol& b)
		{
			if (a.addr != b.addr)
				return a.addr < b.addr;
			return a.size > b.size;
		});
	// 同一地址只保留一个（优先保留有大小的）
	syms_.erase(
		unique(syms_.begin(), syms_.end(), [](const ElfSymbol& a, const ElfSymbol& b) { return a.addr == b.addr; }),
		syms_.end());
	return true;
}

bool ElfFile::offsetToVaddr(uint64_t off, uint64_t& vaddr) const
{
	for (const auto& s : loads_)
	{
		if (off >= s.offset && off < s.offset + s.filesz)
		{
			vaddr = off - s.offset + s.vaddr;
			return true;
		}
	}
	return false;
}

const ElfSymbol* ElfFile::lookup(uint64_t vaddr) const
{
	if (syms_.empty())
		return nullptr;
	auto it = upper_bound(syms_.begin(), syms_.end(), vaddr, [](uint64_t a, const ElfSymbol& s) { return a < s.addr; });
	if (it == syms_.begin())
		return nullptr;
	--it;
	if (it->size > 0)
		return vaddr < it->addr + it->size ? &*it : nullptr;
	// 没有大小信息：认为延伸到下一个符号之前
	auto next = it + 1;
	if (next != syms_.end() && vaddr >= next->addr)
		return nullptr;
	return &*it;
}

// ---------------------------------------------------------------------------
// Symbolizer
// ---------------------------------------------------------------------------
void Symbolizer::addMapping(const Mapping& m)
{
	if (m.path.empty() || m.path[0] != '/')
		return; // 只处理文件映射
	maps_[m.path].push_back(m);
	mappingCount_++;
}

// 示例：
// ApplicationServ  0  0.000000: PERF_RECORD_MMAP2 3575/3575: [0xb04000(0x733000) @ 0 b3:02 934 354382]: r-xp
// /opt/ApplicationServices/ApplicationServices ApplicationServ  0  0.000000: PERF_RECORD_MMAP 3575/3575:
// [0xb04000(0x733000) @ 0]: x /opt/xxx
bool Symbolizer::parseMmapLine(const string& line)
{
	size_t p = line.find("PERF_RECORD_MMAP");
	if (p == string::npos)
		return false;
	try
	{
		size_t sp = line.find(' ', p);
		if (sp == string::npos)
			return true;
		string rest = trim(line.substr(sp + 1));
		// pid/tid:
		Mapping m;
		size_t colon = rest.find(':');
		if (colon != string::npos)
		{
			string pt = rest.substr(0, colon);
			size_t slash = pt.find('/');
			m.pid = stoi(slash == string::npos ? pt : pt.substr(0, slash));
		}
		size_t lb = rest.find('['), lp = rest.find('(', lb), rp = rest.find(')', lp);
		size_t at = rest.find('@', rp), rb = rest.find(']', at);
		if (lb == string::npos || lp == string::npos || rp == string::npos || at == string::npos || rb == string::npos)
			return true;
		m.start = stoull(rest.substr(lb + 1, lp - lb - 1), nullptr, 0);
		uint64_t len = stoull(rest.substr(lp + 1, rp - lp - 1), nullptr, 0);
		m.end = m.start + len;
		istringstream pg(rest.substr(at + 1, rb - at - 1));
		string pgoff;
		pg >> pgoff;
		m.pgoff = stoull(pgoff, nullptr, 0);
		// "]: r-xp /path" 或 "]: x /path"
		string tail = trim(rest.substr(rb + 1));
		if (!tail.empty() && tail[0] == ':')
			tail = trim(tail.substr(1));
		size_t ps = tail.find(' ');
		if (ps == string::npos)
			return true;
		string perms = tail.substr(0, ps);
		m.path = trim(tail.substr(ps + 1));
		if (perms.find('x') == string::npos)
			return true; // 只关心可执行映射
		addMapping(m);
	}
	catch (...)
	{
	}
	return true;
}

// /proc/PID/maps：b6e00000-b6f2a000 r-xp 00000000 b3:02 1234   /lib/arm-linux-gnueabihf/libc-2.30.so
bool Symbolizer::loadMapsFile(const string& file, int pid)
{
	ifstream in(file);
	if (!in)
		return false;
	string line;
	while (getline(in, line))
	{
		istringstream is(line);
		string range, perms, off, dev, inode, path;
		if (!(is >> range >> perms >> off >> dev >> inode))
			continue;
		getline(is, path);
		path = trim(path);
		if (perms.find('x') == string::npos)
			continue;
		size_t dash = range.find('-');
		if (dash == string::npos)
			continue;
		try
		{
			Mapping m;
			m.start = stoull(range.substr(0, dash), nullptr, 16);
			m.end = stoull(range.substr(dash + 1), nullptr, 16);
			m.pgoff = stoull(off, nullptr, 16);
			m.pid = pid;
			m.path = path;
			addMapping(m);
		}
		catch (...)
		{
		}
	}
	return true;
}

ElfFile* Symbolizer::getElf(const string& path)
{
	auto it = cache_.find(path);
	if (it != cache_.end())
		return it->second.get();
	auto ef = std::make_unique<ElfFile>();
	ElfFile* r = nullptr;
	if (ef->load(root_ + path))
		r = ef.get();
	cache_[path] = r ? std::move(ef) : nullptr;
	return r;
}

bool Symbolizer::toVaddr(uint64_t ip, const string& dso, int pid, uint64_t& vaddr)
{
	if (dso.empty() || dso[0] != '/')
		return false;
	ElfFile* ef = getElf(dso);
	if (!ef)
		return false;
	const Mapping* hit = nullptr;
	auto mit = maps_.find(dso);
	if (mit != maps_.end())
	{
		for (const auto& m : mit->second)
		{
			if (ip < m.start || ip >= m.end)
				continue;
			if (!hit || (pid >= 0 && m.pid == pid))
				hit = &m;
			if (pid >= 0 && m.pid == pid)
				break;
		}
	}
	if (hit)
		return ef->offsetToVaddr(ip - hit->start + hit->pgoff, vaddr);
	if (ef->isExecutable())
	{
		vaddr = ip;
		return true;
	}
	return false;
}

bool Symbolizer::lookupFile(const string& path, uint64_t vaddr, string& name, uint64_t& offset)
{
	ElfFile* ef = getElf(path);
	if (!ef)
		return false;
	const ElfSymbol* s = ef->lookup(vaddr);
	if (!s)
		return false;
	name = s->name;
	offset = vaddr - s->addr;
	return true;
}

bool Symbolizer::resolve(uint64_t ip, const string& dso, int pid, string& name)
{
	if (dso.empty() || dso[0] != '/')
	{
		failed_++;
		return false;
	}
	ElfFile* ef = getElf(dso);
	if (!ef)
	{
		failed_++;
		return false;
	}

	// 1. 找到包含该地址的映射（同一库可能在多个进程中映射，优先匹配 pid）
	const Mapping* hit = nullptr;
	auto mit = maps_.find(dso);
	if (mit != maps_.end())
	{
		for (const auto& m : mit->second)
		{
			if (ip < m.start || ip >= m.end)
				continue;
			if (!hit || (pid >= 0 && m.pid == pid))
				hit = &m;
			if (pid >= 0 && m.pid == pid)
				break;
		}
	}

	uint64_t vaddr = 0;
	bool ok = false;
	if (hit)
	{
		uint64_t fileOff = ip - hit->start + hit->pgoff;
		ok = ef->offsetToVaddr(fileOff, vaddr);
	}
	else if (ef->isExecutable())
	{
		vaddr = ip; // 非 PIE 程序：运行地址就是链接地址
		ok = true;
	}
	if (!ok)
	{
		failed_++;
		return false;
	}

	const ElfSymbol* s = ef->lookup(vaddr);
	if (!s)
	{
		failed_++;
		return false;
	}
	name = s->name;
	resolved_++;
	return true;
}

vector<string> Symbolizer::filesOnlyDynsym() const
{
	vector<string> v;
	for (auto& p : cache_)
		if (p.second && !p.second->fromSymtab())
			v.push_back(p.first);
	sort(v.begin(), v.end());
	return v;
}

vector<string> Symbolizer::filesUnreadable() const
{
	vector<string> v;
	for (auto& p : cache_)
		if (!p.second)
			v.push_back(p.first);
	sort(v.begin(), v.end());
	return v;
}
