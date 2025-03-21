#include "utils.h"
#include <errno.h>

char *sf_strdup(const char *s)
{
	char *ptr = NULL;
	ptr = strdup(s);
	if (NULL != ptr)
	{
		// dmp_ale("sf_strdup success,*p:%p\n",ptr);
		return ptr;
	}
	else
		return NULL;
}

// 获取单行数据 ,shell 执行的返回数据是多行，使用上面的 message_from_popen函数
char *sfget_shell(char *cmd)
{
	if (cmd == NULL)
		return NULL;

	FILE *fp = NULL;
	char *ptr = NULL;
	char *tag = NULL;
	char buf[READMAX] = {0};
	memset(buf, 0, sizeof(buf));
	if (NULL != (fp = popen((const char *)cmd, "r")))
	{
		if (NULL != fgets(buf, sizeof(buf) - 1, fp))
		{
			tag = buf;

			while (('\n' != *tag) && ('\0' != *tag))
			{
				tag++;
			}
			*tag = '\0';

			ptr = sf_strdup(buf);
		}
		else
		{
			printf("cmd:%s, err\n", cmd);
		}
		pclose(fp);
		return ptr;
	}
	else
	{
		printf("popen err,cmd:%s,err:%s,errno:%d\n", cmd, strerror(errno), errno);
		return NULL;
	}
}

void sf_free(void *p)
{
	if (NULL != p)
	{
		// dmp_ale("sf_free,*p:%p\n",p);
		free(p);
	}
	else
		printf("sf_free ptr is null\n");
}

unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;
	struct stat statbuff;

	if (stat(path, &statbuff) < 0)
	{
		return filesize;
	}
	else
	{
		filesize = statbuff.st_size;
	}

	return filesize;
}

void delete_file(char *filepath)
{
	char context[256] = {0};
	sprintf(context, "rm -rf %s", filepath);
	system(context);
}
