#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		system("cat /proc/zoneinfo | head -20");
		sleep(1);
	}
}
