// symbol_rules.cpp
#include "symbol_rules.h"
#include "string_utils.h"

#include <vector>

using namespace std;

const unordered_set<string> kLibcNames = {
	"malloc",
	"free",
	"cfree",
	"realloc",
	"calloc",
	"memalign",
	"posix_memalign",
	"_int_malloc",
	"_int_free",
	"_int_realloc",
	"_int_memalign",
	"malloc_consolidate",
	"memcpy",
	"memmove",
	"memset",
	"memcmp",
	"memchr",
	"strlen",
	"strnlen",
	"strcmp",
	"strncmp",
	"strcasecmp",
	"strncasecmp",
	"strcpy",
	"strncpy",
	"strcat",
	"strchr",
	"strrchr",
	"strstr",
	"strtol",
	"strtoul",
	"strtod",
	"atoi",
	"atof",
	"qsort",
	"qsort_r",
	"bsearch",
	"printf",
	"fprintf",
	"sprintf",
	"snprintf",
	"vsnprintf",
	"vfprintf",
	"vsprintf",
	"fwrite",
	"fread",
	"fflush",
	"fputs",
	"puts",
	"fopen",
	"fclose",
	"fgets",
	"gettid",
	"getpid",
	"select",
	"pselect",
	"poll",
	"ppoll",
	"epoll_wait",
	"epoll_pwait",
	"nanosleep",
	"clock_nanosleep",
	"usleep",
	"sleep",
	"sched_yield",
	"clock_gettime",
	"gettimeofday",
	"time",
	"read",
	"write",
	"pread",
	"pwrite",
	"readv",
	"writev",
	"ioctl",
	"fcntl",
	"open",
	"open64",
	"close",
	"lseek",
	"stat",
	"fstat",
	"lstat",
	"stat64",
	"fstat64",
	"recv",
	"recvfrom",
	"recvmsg",
	"send",
	"sendto",
	"sendmsg",
	"accept",
	"connect",
	"socket",
	"bind",
	"listen",
	"sem_wait",
	"sem_trywait",
	"sem_timedwait",
	"sem_post",
	"pthread_mutex_lock",
	"pthread_mutex_unlock",
	"pthread_mutex_trylock",
	"pthread_cond_wait",
	"pthread_cond_timedwait",
	"pthread_cond_signal",
	"pthread_cond_broadcast",
	"pthread_create",
	"pthread_join",
	"start_thread",
	"clone",
	"thread_start",
	"syscall",
	"abort",
	"raise",
};

const unordered_set<string> kSuspiciousLibc = {
	"__fgetspent_r",
	"fgetspent_r",
	"__clearenv",
	"clearenv",
	"envz_entry",
	"getprotobynumber_r",
	"__tdelete",
	"tdelete",
	"__argz_replace",
	"argz_replace",
	"fts_close",
	"__strcasestr",
	"tr_reallochook",
	"__malloc_info.part.9",
	"getpwnam_r",
	"getgrnam_r",
	"__nss_lookup",
	"_nl_find_locale",
};

bool isSyscallEntry(const string& n)
{
	return startsWith(n, "el0_svc") || n == "vector_swi" || n == "ret_fast_syscall" || n == "__sys_trace" ||
		   startsWith(n, "entry_SYSCALL") || n == "do_syscall_64" || startsWith(n, "__arm64_sys_") ||
		   startsWith(n, "__arm64_compat_sys_") || startsWith(n, "__x64_sys_") || startsWith(n, "__ia32_sys_") ||
		   // 32 位 ARM 等架构的内核里，系统调用实现直接命名为 sys_xxx
		   startsWith(n, "sys_") || startsWith(n, "__se_sys_") || startsWith(n, "__do_sys_") ||
		   startsWith(n, "compat_sys_");
}

bool isExceptionEntry(const string& n)
{
	static const vector<string> names = {
		"__irq_usr",
		"__irq_svc",
		"__dabt_usr",
		"__pabt_usr",
		"__und_usr",
		"do_DataAbort",
		"do_PrefetchAbort",
		"do_page_fault",
		"__do_page_fault",
		"do_translation_fault",
		"gic_handle_irq",
		"handle_IRQ",
		"asm_do_IRQ",
		"__handle_domain_irq",
		"el0_irq",
		"el0_da",
		"el0_ia",
		"el1_irq",
		"el0_sync_compat_handler_da",
		"do_mem_abort",
		"irq_exit",
		"__do_softirq",
		"asm_exc_page_fault",
		"exc_page_fault",
		"common_interrupt",
	};
	for (const auto& x : names)
		if (n == x)
			return true;
	return false;
}

string kernelArchHint(const string& n)
{
	if (startsWith(n, "el0_") || startsWith(n, "el1_") || startsWith(n, "__arm64_"))
		return "arm64";
	if (n == "vector_swi" || n == "ret_fast_syscall" || n == "__irq_usr" || n == "__irq_svc" || n == "__dabt_usr" ||
		n == "__pabt_usr" || n == "ret_to_user" || n == "ret_to_user_from_irq")
		return "arm";
	if (startsWith(n, "entry_SYSCALL") || n == "do_syscall_64" || startsWith(n, "__x64_sys_"))
		return "x86_64";
	return "";
}

bool looksLikeKernel(const string& n)
{
	static const vector<string> prefixes = {
		"el0_",
		"el1_",
		"__arm64_",
		"do_",
		"sys_",
		"__sys_",
		"ksys_",
		"__se_sys_",
		"compat_",
		"__compat_",
		"schedule",
		"__schedule",
		"_raw_spin",
		"_raw_read",
		"_raw_write",
		"finish_task_switch",
		"hrtimer_",
		"ktime_",
		"futex_",
		"__fget",
		"__fdget",
		"fget",
		"fpsimd_",
		"vector_swi",
		"ret_fast_syscall",
		"entry_SYSCALL",
		"irq_",
		"gic_",
		"__handle_",
		"handle_",
		"__do_softirq",
		"tick_",
		"__task_pid_nr_ns",
		"copy_to_user",
		"copy_from_user",
		"__copy_",
		"__arch_copy",
		"vfs_",
		"path_",
		"filename_lookup",
		"user_path",
		"lookup_",
		"__d_lookup",
		"d_alloc",
		"kmem_cache",
		"__kmalloc",
		"kfree",
		"__wake_up",
		"try_to_wake_up",
		"wake_up",
		"pick_next_task",
		"update_curr",
		"enqueue_",
		"dequeue_",
		"account_",
		"rcu_",
		"__rcu",
		"mutex_",
		"__mutex",
		"down_",
		"up_read",
		"up_write",
		"page_fault",
		"do_page_fault",
		"__do_page_fault",
		"handle_mm_fault",
		"__handle_mm_fault",
		"el0_da",
		"el0_ia",
		"sock_",
		"tcp_",
		"udp_",
		"ip_",
		"__sock",
		"inet_",
		"poll_",
		"pollwake",
		"__pollwait",
		"unix_",
		"pipe_",
		"eventfd_",
		"ep_",
		"timerqueue_",
		"clockevents_",
		"arch_timer",
		"cpuidle",
		"__softirqentry",
		"ttwu_",
		"check_preempt",
		"resched_",
		"native_",
		"posix_",
		"common_nsleep",
		"nanosleep_",
		"seq_",
		"proc_",
		"kernfs_",
		"__audit",
		"audit_",
		"security_",
		"selinux_",
		"apparmor_",
		"refcount_",
		"kref_",
		"fput",
		"fd_install",
		"__close_fd",
		"ksize",
		"memcg_",
		"mod_timer",
		"add_timer",
	};
	for (const auto& p : prefixes)
		if (startsWith(n, p))
			return true;
	return false;
}

bool isRootAnomaly(const string& n) { return startsWith(n, "[perf-") || startsWith(n, "[JIT") || n == "[unknown]"; }

bool isAddrFrame(const string& n)
{
	size_t p = n.rfind("+0x");
	return p != string::npos && p > 0 && n.find('(') == string::npos && n.find("::") == string::npos &&
		   n.find_first_not_of("0123456789abcdefABCDEF", p + 3) == string::npos;
}

bool isSystemLibFile(const string& f)
{
	static const vector<string> prefixes = {
		"libc-",
		"libc.so",
		"libpthread",
		"libstdc++",
		"libgcc_s",
		"ld-linux",
		"ld-",
		"librt",
		"libm-",
		"libm.so",
		"libdl",
		"libglib",
		"libgobject",
		"libgio",
		"libdbus",
		"libz.so",
		"libssl",
		"libcrypto",
		"libtdb",
		"libjson-glib",
		"libnss",
		"libresolv",
		"libuuid",
		"libsystemd",
		"libutil",
	};
	for (const auto& p : prefixes)
		if (startsWith(f, p))
			return true;
	return false;
}

bool isUnresolved(const string& n) { return (!n.empty() && n[0] == '[') || startsWith(n, "0x") || isAddrFrame(n); }

bool isLibName(const string& raw)
{
	// 地址帧：系统库的算库函数，其余（程序本身和自研库）算业务代码
	if (isAddrFrame(raw))
		return isSystemLibFile(raw.substr(0, raw.rfind("+0x")));
	string n = stripVersion(raw);
	if (n.empty())
		return true;
	if (n[0] == '[')
		return true;
	if (startsWith(n, "0x"))
		return true;
	if (startsWith(n, "std::") || startsWith(n, "__gnu_cxx::") || startsWith(n, "__cxxabiv1::") ||
		startsWith(n, "operator new") || startsWith(n, "operator delete") || startsWith(n, "typeinfo") ||
		startsWith(n, "void std::") || startsWith(n, "bool std::") || startsWith(n, "__cxa_"))
		return true;
	if (startsWith(n, "_"))
		return true; // __xxx / _int_malloc / _dbus_xxx 等内部函数
	static const vector<string> libPrefixes = {
		"dbus_",
		"g_",
		"tdb_",
		"sqlite3",
		"lua_",
		"luaL_",
		"luaV_",
		"luaD_",
		"xml",
		"json_object",
		"json_tokener",
		"SSL_",
		"ssl3_",
		"EVP_",
		"CRYPTO_",
		"BIO_",
		"z_",
		"inflate",
		"deflate",
	};
	for (const auto& p : libPrefixes)
		if (startsWith(n, p))
			return true;
	if (kLibcNames.count(n))
		return true;
	return false;
}
