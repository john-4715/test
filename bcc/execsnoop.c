#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void get_cmdline(pid_t pid, char *buf, int sz)
{
	char path[64];
	snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
	FILE *f = fopen(path, "r");
	if (!f)
	{
		snprintf(buf, sz, "(unknown)");
		return;
	}
	fgets(buf, sz, f);
	fclose(f);
	for (char *p = buf; *p; p++)
		if (*p == 0)
			*p = ' ';
}

static int is_ignored(const char *cmd)
{
	if (strstr(cmd, "-bash") != NULL)
		return 1;
	if (strstr(cmd, "bash") != NULL && strlen(cmd) < 8)
		return 1;
	if (strlen(cmd) < 3)
		return 1;
	return 0;
}

static void scan(void)
{
	DIR *d = opendir("/proc");
	if (!d)
		return;
	struct dirent *ent;

	while ((ent = readdir(d)))
	{
		int pid = atoi(ent->d_name);
		if (pid <= 0)
			continue;

		char cmd[1024];
		get_cmdline(pid, cmd, sizeof(cmd));

		if (is_ignored(cmd))
			continue;
		printf("%-8d %s\n", pid, cmd);
	}

	closedir(d);
}

int main()
{
	while (1)
	{
		// 清屏
		system("clear");

		printf("PID     COMMAND\n");
		printf("------------------------------\n");

		// 扫描一次
		scan();

		// 每隔 10 秒刷新
		sleep(10);
	}
	return 0;
}