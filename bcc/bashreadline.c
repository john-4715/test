#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		system("tail -n 10 ~/.bash_history 2>/dev/null");
		sleep(1);
	}
}
