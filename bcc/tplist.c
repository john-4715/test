#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	system("ls /sys/kernel/debug/tracing/events 2>/dev/null | head -40");
	return 0;
}
