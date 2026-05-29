#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		printf("=== TCP STATES ===\n");
		system("cat /proc/net/tcp /proc/net/tcp6 2>/dev/null | awk '{print $4}' | sort | uniq -c");
		sleep(1);
	}
}
