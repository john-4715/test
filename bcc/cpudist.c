#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		system("cat /proc/stat | head -5");
		sleep(1);
	}
}
