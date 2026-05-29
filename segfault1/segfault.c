#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void test1(int number)
{
	char *p = (char *)number;
	*p = 's';
	printf("%s: test1\n", __FUNCTION__);
}

void DebugBacktrace(void)
{
#define SIZE 100
	void *array[SIZE];
	int size, i;
	char **strings;

	fprintf(stderr, "\n Segmentation fault \n");
	size = backtrace(array, SIZE);
	fprintf(stderr, "Backtrace (%d deef):\n", size);

	strings = backtrace_symbols(array, size);

	for (i = 0; i < size; i++)
		fprintf(stderr, "%d: %s\n", i, strings[i]);

	free(strings);
	exit(-1);
}

int main(void)
{
	char data = 1;

	signal(SIGSEGV, DebugBacktrace);
	test1(data);
}
