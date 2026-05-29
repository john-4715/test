#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		printf("=== OPEN FILES TOP ===\n");
		system("lsof 2>/dev/null | head -40");
		sleep(1);
	}
}
