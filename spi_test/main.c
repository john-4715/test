#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "spi_flash.h"

static void print_tip(void)
{
	printf("./flash read  [read.txt]   ----read flash to read.txt \r\n");
	printf("./flash write [write.txt]  ----write write.txt to flash and read flash to read.txt\r\n");
	printf("./flash clean              ----flash earse \r\n");
}

int main(int argc, char *argv[])
{
	int fd;
	unsigned char tx_buf[10] = {0};
	unsigned char rx_buf[sizeof(tx_buf)];
	int ret;

	ret = SPI_Open("/dev/spidev0.0", &fd);
	if (fd < 0)
	{
		printf("open spi dev failure.\n");
		return -1;
	}

#if 1
	ret = SPI_LookBackTest(fd);
	// for (int i = 0; i < 10; i++)
	// {
	// 	tx_buf[i] = i;
	// }
	// SPI_Write(fd, tx_buf, 10);
	// SPI_Read(fd, rx_buf, 10);

#else
	int read_filefd, write_filefd, device_fd;
	const char *str_cmd = NULL;
	char *read_fileName = NULL;
	char *write_fileName = NULL;
	char *device = "/dev/mtd4";

	char cmd_sh[128];

	if (3 == argc)
	{
		str_cmd = argv[1];
		switch (*str_cmd)
		{
		case 'w': // write
			write_fileName = argv[2];
			device_fd = open(device, O_RDWR);
			if (device_fd < 0)
			{
				printf("can't open %s\n", device);
				return -1;
			}
			write_filefd = open(write_fileName, O_RDWR);
			if (write_filefd < 0)
			{
				printf("can't open %s\n", read_filefd);
				return -1;
			}

			flash_write(device_fd, write_filefd, FLASH_MAX_SIZE);

			close(write_filefd);
			close(device_fd);
			break;
		case 'r': // read
			read_fileName = argv[2];
			device_fd = open(device, O_RDWR);
			if (device_fd < 0)
			{
				printf("can't open %s\n", device);
				return -1;
			}
			read_filefd = open(read_fileName, O_RDWR);
			if (read_filefd < 0)
			{
				printf("can't open %s\n", read_filefd);
				return -1;
			}

			flash_read(device_fd, read_filefd, FLASH_MAX_SIZE);

			close(read_filefd);
			close(device_fd);
			break;
		default:
			print_tip();
			break;
		}
	}
	else if (2 == argc)
	{
		str_cmd = argv[1];
		switch (*str_cmd)
		{
		case 'c': // clean
			sprintf(cmd_sh, "flash_erase /dev/mtd4 0 0");
			execl("/bin/sh", "sh", "-c", cmd_sh, (char *)0);
			printf("flash clean earse\n");
			break;
		default:
			print_tip();
			break;
		}
	}
	else
	{
		print_tip();
	}

	return 0;
#endif

	SPI_Close(fd);
	return 0;
}