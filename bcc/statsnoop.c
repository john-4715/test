#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	printf("PID     READ_BYTES  WRITE_BYTES\n");
	while (1)
	{
		system("for p in /proc/[0-9]*; do cat $p/io 2>/dev/null | grep -A1 'read_bytes' | xargs; done");
		sleep(1);
	}
}
