#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	char prev[8192] = {0};
	while (1)
	{
		char curr[8192];
		FILE *f = popen("mount", "r");
		if (f)
		{
			fread(curr, 1, sizeof(curr), f);
			pclose(f);
		}
		if (strcmp(prev, curr) != 0)
			printf("=== MOUNTS CHANGED ===\n");
			system("mount");
			strcpy(prev, curr);
		sleep(1);
	}
}
