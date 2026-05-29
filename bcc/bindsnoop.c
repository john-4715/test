#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
	while (1)
	{
		printf("=== BINDED PORTS ===\n");
		system("cat /proc/net/tcp /proc/net/tcp6 2>/dev/null | grep '0A'");
		sleep(1);
	}
}