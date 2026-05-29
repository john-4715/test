#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		system("dmesg | grep -i oom | tail -10");
		sleep(2);
	}
}
