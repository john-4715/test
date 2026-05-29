#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		printf("=== TCP RTT ===\n");
		system("cat /proc/net/tcp | grep -v 'rem_addr' | awk '{print $15}'");
		sleep(1);
	}
}
