#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		printf("=== TCP ESTABLISHED ===\n");
		system("cat /proc/net/tcp | grep ESTABLISHED");
		sleep(1);
	}
}
