#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#define EMIO_GPIO_DIR "/sys/class/gpio"
#define EMIO_PIN_BASE 426
#define EMIO_PIN_NUM 1 // 假设你使用的是第一个EMIO引脚

int main(int argc, char *argv[])
{

	int ret;
	int fd;
	char buffer[64];

	for (int pin_num = 0; pin_num < 64; pin_num++)
	{
		// 配置EMIO为输出
		sprintf(buffer, "%s/export", EMIO_GPIO_DIR);
		printf("%s\n", buffer);
		fd = open(buffer, O_WRONLY);
		if (fd < 0)
		{
			perror("open export for writing");
			return -1;
		}
		sprintf(buffer, "%d", EMIO_PIN_BASE + pin_num);
		printf("%s\n", buffer);
		ret = write(fd, buffer, sizeof(buffer));
		if (ret < 0)
		{
			perror(buffer);
			return -1;
		}
		close(fd);

		// 设置GPIO方向
		sprintf(buffer, "%s/gpio%d/direction", EMIO_GPIO_DIR, EMIO_PIN_BASE + pin_num);
		printf("%s\n", buffer);
		fd = open(buffer, O_RDWR);
		if (fd < 0)
		{
			perror("open direction for reading and writing");
			return -1;
		}
		sprintf(buffer, "%s", "out");
		printf("%s\n", buffer);
		ret = write(fd, buffer, 3); // 设置为输出
		if (ret < 0)
		{
			perror(buffer);
			return -1;
		}
		close(fd);

		// 写入EMIO引脚值
		sprintf(buffer, "%s/gpio%d/value", EMIO_GPIO_DIR, EMIO_PIN_BASE + pin_num);
		printf("%s\n", buffer);
		fd = open(buffer, O_RDWR);
		if (fd < 0)
		{
			perror("open value for reading and writing");
			return -1;
		}
		for (int j = 0; j < 10; j++)
		{
			// 输出高电平
			sprintf(buffer, "%s", "1");
			ret = write(fd, buffer, 1);
			printf("%s\n", buffer);
			if (ret < 0)
			{
				perror(buffer);
				return -1;
			}
			// 延时一段时间
			sleep(1);
			// 输出低电平
			sprintf(buffer, "%s", "0");
			printf("%s\n", buffer);
			ret = write(fd, buffer, 1);
			if (ret < 0)
			{
				perror(buffer);
				return -1;
			}
			sleep(1);
		}
		close(fd);

		if (0)
		{
			// 取消EMIO引脚的导出
			sprintf(buffer, "%s/unexport", EMIO_GPIO_DIR);
			printf("%s\n", buffer);
			fd = open(buffer, O_WRONLY);
			if (fd < 0)
			{
				perror("open unexport for writing");
				return 1;
			}
			sprintf(buffer, "%d", EMIO_PIN_BASE + pin_num);
			printf("%s\n", buffer);
			ret = write(fd, buffer, sizeof(buffer));
			if (ret < 0)
			{
				perror(buffer);
				return -1;
			}
			close(fd);
		}
	}

	return 0;
}
