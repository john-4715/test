#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	// 执行shell命令
	FILE *fp = popen("ls", "r"); // 例如，执行ls命令
	if (fp == NULL)
	{
		perror("popen failed");
		return -1;
	}

	char buffer[1024];
	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		// 去除行尾的换行符
		buffer[strcspn(buffer, "\n")] = 0;

		// 检查是否为空行
		if (strlen(buffer) > 0)
		{ // 如果行非空，则打印
			printf("%s\n", buffer);
		}
	}

	// 关闭流
	if (pclose(fp) == -1)
	{
		perror("pclose failed");
		return -1;
	}

	return 0;
}
