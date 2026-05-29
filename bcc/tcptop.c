#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	while (1)
	{
		printf("=== TCP TOP ===\n");
		system("ss -tulnp 2>/dev/null | head -30");
		sleep(1);
	}
}
