#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX 8192
static int before[MAX] = {0};

static void load_pids(int *buf)
{
	memset(buf, 0, sizeof(int) * MAX);
	DIR *d = opendir("/proc");
	if (!d)
		return;
	int i = 0;
	struct dirent *e;
	while ((e = readdir(d)) && i < MAX)
	{
		int pid = atoi(e->d_name);
		if (pid > 0)
			buf[i++] = pid;
	}
	closedir(d);
}

int main()
{
	load_pids(before);
	while (1)
	{
		sleep(1);
		int curr[MAX] = {0};
		load_pids(curr);
		for (int a = 0; a < MAX; a++)
		{
			int p = before[a];
			if (!p)
				continue;
			int f = 0;
			for (int b = 0; b < MAX; b++)
				if (curr[b] == p)
				{
					f = 1;
					break;
				}
			if (!f)
				printf("EXIT %d\n", p);
		}
		memcpy(before, curr, sizeof(before));
	}
	return 0;
}