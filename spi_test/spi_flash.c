#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "spi_flash.h"

#define SPI_DEBUG 1

// static const char *device = "/dev/spidev2.0";
static uint8_t mode = 0;		 /* SPI通信使用全双工，设置CPOL＝0，CPHA＝0。 */
static uint8_t bits = 8;		 /* ８ｂiｔｓ读写，MSB first。*/
static uint32_t speed = 5000000; /* 设置5M传输速度 */
static uint16_t delay = 0;

unsigned char r_buf[256];
unsigned char w_buf[256];

#define SPI_CS0 0
#define SPI_CS1 1

static void pabort(const char *s)
{
	perror(s);
	// abort();
}

int SPI_Open(const char *device, int *spi_fd)
{
	int fd;
	int ret = 0;

	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		pabort("can't open device");
		*spi_fd = -1;
		return -1;
	}
	else
	{
		printf("SPI - Open Succeed. Start Init SPI...\n");
	}

	*spi_fd = fd;
	/*
	 * spi mode
	 */
	// mode |= SPI_CS_HIGH;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d KHz (%d MHz)\n", speed / 1000, speed / 1000 / 1000);

	return ret;
}

int SPI_Close(int fd)
{
	if (fd == 0) /* SPI是否已经打开*/
		return 0;
	close(fd);

	return 0;
}

#define SPI_SLAVE_NUMBER 1

int SPI_Transfer(int fd, const uint8_t *TxBuf, uint8_t *RxBuf, int len)
{
	int ret;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)TxBuf,
		.rx_buf = (unsigned long)RxBuf,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (ret < 0)
	{
		// 错误处理
		close(fd);
		return -1;
	}
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
	{
		perror("can't send spi message");
	}
	else
	{
#if SPI_DEBUG
		int i;
		printf("\nsend spi message Succeed");
		printf("\nSPI Send [Len:%d]: ", len);
		for (i = 0; i < len; i++)
		{
			if (i % 8 == 0)
				printf("\n\t");
			printf("0x%02X ", TxBuf[i]);
		}
		printf("\n");

		printf("SPI Receive [len:%d]:", len);
		for (i = 0; i < len; i++)
		{
			if (i % 8 == 0)
				printf("\n\t");
			printf("0x%02X ", RxBuf[i]);
		}
		printf("\n");
#endif
	}

	if (ret < 0)
	{
		// 错误处理
		close(fd);
		return -1;
	}
	return ret;
}

int SPI_Write(int fd, uint8_t *TxBuf, int len)
{
	int ret;

	ret = write(fd, TxBuf, len);
	if (ret < 0)
		perror("SPI Write error\n");
	else
	{
#if SPI_DEBUG
		int i;
		printf("\nSPI Write [Len:%d]: ", len);
		for (i = 0; i < len; i++)
		{
			if (i % 10 == 0)
				printf("\n\t");
			printf("0x%02X ", TxBuf[i]);
		}
		printf("\n");

#endif
	}

	return ret;
}

int SPI_Read(int fd, uint8_t *RxBuf, int len)
{
	int ret;

	ret = read(fd, RxBuf, len);
	if (ret < 0)
		perror("SPI Read error\n");
	else
	{
#if SPI_DEBUG
		int i;
		printf("SPI Read [len:%d]:", len);
		for (i = 0; i < len; i++)
		{
			if (i % 8 == 0)
				printf("\n\t");
			printf("0x%02X ", RxBuf[i]);
		}
		printf("\n");
#endif
	}

	return ret;
}

int SPI_LookBackTest(int fd)
{
	int ret, i;
	const int BufSize = 16;
	uint8_t tx[BufSize], rx[BufSize];

	bzero(rx, sizeof(rx));
	for (i = 0; i < BufSize; i++)
		tx[i] = i;

	printf("\nSPI - LookBack Mode Test...\n");
	ret = SPI_Transfer(fd, tx, rx, BufSize);
	if (ret > 1)
	{
		ret = memcmp(tx, rx, BufSize);
		if (ret != 0)
		{
			perror("LookBack Mode Test error\n");
			// pabort("error");
		}
		else
			printf("SPI - LookBack Mode  OK\n");
	}

	return ret;
}

int flash_write(int device_fd, int write_filefd, unsigned int length)
{
	int bit = length % FLASH_PAGE_SIZE;
	int page = length / FLASH_PAGE_SIZE;
	unsigned char filebuf[FLASH_PAGE_SIZE];

	while (page--)
	{
		read(write_filefd, filebuf, FLASH_PAGE_SIZE);
		for (int i = 0; i < length; i++)
		{
			if (i % 8 == 0)
				printf("\n\t");
			printf("0x%02X ", filebuf[i]);
		}
		write(device_fd, filebuf, FLASH_PAGE_SIZE);
	}
	if (bit)
	{
		read(write_filefd, filebuf, bit);
		write(device_fd, filebuf, bit);
	}

	return 0;
}

int flash_read(int device_fd, int read_filefd, unsigned int length)
{
	int bit = length % FLASH_PAGE_SIZE;
	int page = length / FLASH_PAGE_SIZE;
	unsigned char filebuf[FLASH_PAGE_SIZE];

	printf("bit  = %d\n", bit);
	printf("page = %d\n", page);

	while (page--)
	{
		read(device_fd, filebuf, FLASH_PAGE_SIZE);
		for (int i = 0; i < length; i++)
		{
			if (i % 8 == 0)
				printf("\n\t");
			printf("0x%02X ", filebuf[i]);
		}
		write(read_filefd, filebuf, FLASH_PAGE_SIZE);
	}
	if (bit)
	{
		read(device_fd, filebuf, bit);
		write(read_filefd, filebuf, bit);
	}

	return 0;
}
