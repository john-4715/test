#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		printf("=== SWAP USAGE ===\n");
		system("cat /proc/meminfo | grep -i swap");
		system("for p in /proc/[0-9]*; do cat $p/stat 2>/dev/null | awk '{print $1,$10}'; done | head -20");
		sleep(1);
	}
}
