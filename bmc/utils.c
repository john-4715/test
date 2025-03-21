#include "utils.h"

unsigned int crc_table[256];

void init_crc32_table(void)
{
	unsigned int c;
	unsigned int i, j;

	for (i = 0; i < 256; i++)
	{
		c = (unsigned int)i;
		for (j = 0; j < 8; j++)
		{
			if (c & 1)
			{
				c = 0xedb88320L ^ (c >> 1);
			}
			else
			{
				c = c >> 1;
			}
		}
		crc_table[i] = c;
	}
}

/*计算buffer的crcУ校验码*/
unsigned int crc32_cal(unsigned int crc, unsigned char *buffer, unsigned int size)
{
	unsigned int i;
	for (i = 0; i < size; i++)
	{
		crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
	}
	return crc;
}

/*字母转小写*/
static int to_lower(int m)
{
	if (m >= 'A' && m <= 'Z')
	{
		return m + 'a' - 'A';
	}
	else
	{
		return m;
	}
}

// 将十六进制的字符串转换成对应的整数
int hstrtoi(char *s)
{
	int i = 0;
	int n = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
	{
		i = 2;
	}
	else
	{
		i = 0;
	}
	for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'); ++i)
	{
		if (to_lower(s[i]) > '9')
		{
			n = 16 * n + (10 + to_lower(s[i]) - 'a');
		}
		else
		{
			n = 16 * n + (to_lower(s[i]) - '0');
		}
	}
	return n;
}

uint64_t hstrtol(char *s)
{
	int i = 0;
	uint64_t n = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
	{
		i = 2;
	}
	else
	{
		i = 0;
	}
	for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'); ++i)
	{
		if (to_lower(s[i]) > '9')
		{
			n = 16 * n + (10 + to_lower(s[i]) - 'a');
		}
		else
		{
			n = 16 * n + (to_lower(s[i]) - '0');
		}
	}
	return n;
}

void getLocalTime(CUR_TIME_PARA *curtime)
{
	time_t currentTime; // 存储当前时间的变量
	struct tm *localTime;
	// 获取当前时间
	currentTime = time(NULL);

	// 将currentTime转换为本地时间格式
	localTime = localtime(&currentTime);

	curtime->year = localTime->tm_year + 1900; // 从tm结构体中获取年份（需要加上1900）
	curtime->month = localTime->tm_mon + 1;	   // 从tm结构体中获取月份（注意月份范围是0-11）
	curtime->day = localTime->tm_mday;
	curtime->hour = localTime->tm_hour;
	curtime->min = localTime->tm_min;
	curtime->sec = localTime->tm_sec;
}

double GetCurrentTime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (double)tv.tv_sec + (double)tv.tv_usec / (double)1000000;
}

double GetMilliTime()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / (double)1000;
}

// 创建目录名（先判断存不存在）
int file_exist(const char *path)
{
	char DirName[256];
	strcpy(DirName, path);
	int i, len = strlen(DirName);
	for (i = 1; i < len; i++)
	{
		if (DirName[i] == '/')
		{
			DirName[i] = 0;
			if (access(DirName, F_OK) != 0)
			{
				if (mkdir(DirName, 0755) == -1)
				{
					printf("mkdir   error\n");
					return -1;
				}
			}
			DirName[i] = '/';
		}
	}

	return 0;
}
