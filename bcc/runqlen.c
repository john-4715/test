#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		system("cat /proc/schedstat | head -10");
		sleep(1);
	}
}
