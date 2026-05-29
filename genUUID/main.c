#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// 从最佳可用源获取随机字节
void get_random_bytes(unsigned char *buf, size_t len)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0)
	{
		read(fd, buf, len);
		close(fd);
	}
	else
	{
		// 后备方案 - 使用时间+pid作为种子
		srand(time(NULL) ^ getpid());
		for (size_t i = 0; i < len; i++)
		{
			buf[i] = rand() % 256;
		}
	}
}

// 生成UUID版本4 (随机)
void generate_uuid_v4(char *uuid_str)
{
	unsigned char bytes[16];
	get_random_bytes(bytes, 16);

	// 设置版本号 (4) 和变体 (10)
	bytes[6] = (bytes[6] & 0x0F) | 0x40; // 版本4
	bytes[8] = (bytes[8] & 0x3F) | 0x80; // 变体1 (RFC 4122)

	// 格式化为UUID字符串
	snprintf(uuid_str, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", bytes[0], bytes[1],
			 bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11],
			 bytes[12], bytes[13], bytes[14], bytes[15]);
}

int main()
{
	char uuid[37]; // 36字符 + null终止符
	generate_uuid_v4(uuid);
	printf("Generated UUID: %s\n", uuid);

	// 示例输出: 6d1ee0b2-bbcb-4c02-a7f9-55f3838bde79
	return 0;
}