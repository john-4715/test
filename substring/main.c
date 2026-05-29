#include <stdio.h>
#include <string.h>

int main()
{
	char str[80] = "This is a [sample] string.";
	char delims[] = "[]";
	char *result = NULL;

	// 找到第一个'['对应的']'
	result = strtok(str, delims);
	int i = 0;
	// 循环直到找不到更多的token
	while (result != NULL)
	{
		if (i > 0)
		{
			printf("%s\n", result);
			break;
		}

		// 查找下一个token
		result = strtok(NULL, delims);
		i++;
	}

	return 0;
}