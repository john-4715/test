#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX 8192
static int cache[MAX] = {0};

static void scan()
{
	DIR *d = opendir("/proc");
	if (!d)
		return;
	struct dirent *e;
	while ((e = readdir(d)))
	{
		int pid = atoi(e->d_name);
		if (pid <= 0)
			continue;
		char path[64];
		snprintf(path, sizeof(path), "/proc/%d/task", pid);
		DIR *tdir = opendir(path);
		if (!tdir)
			continue;
		struct dirent *t;
		while ((t = readdir(tdir)))
		{
			int tid = atoi(t->d_name);
			if (tid <= 0)
				continue;
			int found = 0;
			for (int i = 0; i < MAX; i++)
				if (cache[i] == tid)
				{
					found = 1;
					break;
				}
			if (!found)
			{
				printf("THREAD PID=%d TID=%d\n", pid, tid);
				for (int i = 0; i < MAX; i++)
					if (!cache[i])
					{
						cache[i] = tid;
						break;
					}
			}
		}
		closedir(tdir);
	}
	closedir(d);
}

int main()
{
	while (1)
	{
		scan();
		usleep(50000);
	}
	return 0;
}