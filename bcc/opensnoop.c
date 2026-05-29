#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FD 16384
struct
{
	int pid;
	int fd;
	char path[256];
} fd_cache[MAX_FD] = {0};

static void check_fd(int pid, int fd)
{
	char link[256], path[256];
	snprintf(link, sizeof(link), "/proc/%d/fd/%d", pid, fd);
	ssize_t n = readlink(link, path, sizeof(path) - 1);
	if (n <= 0)
		return;
	path[n] = 0;
	if (strstr(path, "socket:"))
		return;
	for (int i = 0; i < MAX_FD; i++)
		if (fd_cache[i].pid == pid && fd_cache[i].fd == fd)
			return;
	for (int i = 0; i < MAX_FD; i++)
	{
		if (!fd_cache[i].pid)
		{
			fd_cache[i].pid = pid;
			fd_cache[i].fd = fd;
			strncpy(fd_cache[i].path, path, 255);
			printf("OPEN %-8d %s\n", pid, path);
			break;
		}
	}
}

static void scan(void)
{
	DIR *proc = opendir("/proc");
	if (!proc)
		return;
	struct dirent *e;
	while ((e = readdir(proc)))
	{
		int pid = atoi(e->d_name);
		if (pid <= 0)
			continue;
		char dir[64];
		snprintf(dir, sizeof(dir), "/proc/%d/fd", pid);
		DIR *d = opendir(dir);
		if (!d)
			continue;
		struct dirent *fd_ent;
		while ((fd_ent = readdir(d)))
		{
			int fd = atoi(fd_ent->d_name);
			if (fd > 0)
				check_fd(pid, fd);
		}
		closedir(d);
	}
	closedir(proc);
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