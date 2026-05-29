#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	while (1)
	{
		system("ps aux | wc -l");
		sleep(1);
	}
}
