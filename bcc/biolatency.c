#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	while (1)
	{
		printf("=== DISK LATENCY ===\n");
		system("cat /sys/block/*/stat 2>/dev/null | head -30");
		sleep(1);
	}
}
