#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
	while (1)
	{
		printf("=== DISK IO ===\n");
		system("iostat -d -x 1 1 | grep -v '^Linux' | grep -v '^$'");
		sleep(1);
	}
}