#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	while (1)
	{
		printf("=== TCP CONNECTIONS ===\n");
		system("cat /proc/net/tcp /proc/net/tcp6 2>/dev/null | grep -v '0A' | grep -v '00000000:0000'");
		sleep(1);
	}
}
