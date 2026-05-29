
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h> // 缺失这个call trace 信息会丢失
#include <string>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>

#include "test.h"


/* SIGSEGV信号的处理函数，回溯栈，打印函数的调用关系 */
void DebugBacktrace(int signo)
{
	// #define SIZE 100
	// 	void *array[SIZE];
	// 	int size, i;
	// 	char **strings;

	// 	fprintf(stderr, "\nSegmentation fault\n");
	// 	size = backtrace(array, SIZE);
	// 	fprintf(stderr, "Backtrace (%d deep):\n", size);
	// 	strings = backtrace_symbols(array, size);
	// 	for (i = 0; i < size; i++)
	// 	{
	// 		fprintf(stderr, "%d: %s\n", i, strings[i]);
	// 	}

	// 	free(strings);

	char cmd[1024] = {0};
	sprintf(cmd, "gdb --batch  -ex 'thread apply all backtrace' --pid=%d", getpid());
	system(cmd);

	char buff[64] = {0};
	sprintf(buff, "cat /proc/%d/maps", getpid());
	system(buff);

	exit(-1);
}


int main(int argc, char **argv)
{
	setenv("LD_BIND_NOW", "1", 1);

	/* 设置SIGSEGV信号的处理函数 */
	signal(SIGSEGV, DebugBacktrace);
    signal(SIGHUP, DebugBacktrace);
    signal(SIGINT, DebugBacktrace);
    signal(SIGPIPE, DebugBacktrace);
    signal(SIGALRM, DebugBacktrace);
    signal(SIGTERM, DebugBacktrace);
    signal(SIGUSR1, DebugBacktrace);
    signal(SIGUSR2, DebugBacktrace);
    signal(SIGQUIT, DebugBacktrace);
    signal(SIGILL, DebugBacktrace);
    signal(SIGABRT, DebugBacktrace);
    signal(SIGFPE, DebugBacktrace);
    signal(SIGBUS, DebugBacktrace);
    signal(SIGSYS, DebugBacktrace);

	// int result = func_add(1, 2);
	json_noexist();

	return 0;
}
