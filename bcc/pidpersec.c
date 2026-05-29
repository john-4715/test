#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	int last = 0;
	while (1)
	{
		int now = 0;
		FILE *f = popen("ps aux | wc -l", "r");
		fscanf(f, "%d", &now);
		pclose(f);
		printf("PID/s: %d\n", now - last);
		last = now;
		sleep(1);
	}
}
