#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int main()
{
	printf("PID     COMM\n");
	while (1)
	{
		system("for p in /proc/[0-9]*; do grep -a sync $p/syscall 2>/dev/null | head -1; done");
		sleep(1);
	}
}
