#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

const char *i2c_dev = "/dev/i2c-0";

int main()
{
	int fd = 0;
	unsigned char slave_add = 0x08;
	char i2c_tmp[5] = {0x03, 0x00, 0x00, 0x30, 0x33};
	char rbuf[20];
	int i;
	/* 打开设备文件 */
	fd = open(i2c_dev, O_RDWR);
	if (fd < 0)
	{
		perror("open i2c device error\n");
	}
	/* 设置为7bit地址 */
	if (ioctl(fd, I2C_TENBIT, 0) < 0)
	{
		perror("Unable to set I2C_TENBIT");
		return 1;
	}
	/* 设置从机地址 */
	if (ioctl(fd, I2C_SLAVE, slave_add >> 1) < 0)
	{
		perror("Unable to set I2C_SLAVE");
		return 1;
	}
	/* 发送数据 */
	write(fd, i2c_tmp, 5);
	usleep(12000);
	/* 读取数据 */
	read(fd, rbuf, 20);
	for (i = 0; i < 20; i++)
	{
		printf("----i2c_read-buff:%02x-----\n", rbuf[i]);
	}

	close(fd);

	return 0;
}
