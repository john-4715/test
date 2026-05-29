#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		system("cat /proc/net/tcp | awk '{print $12}' | head -20");
		sleep(1);
	}
}
