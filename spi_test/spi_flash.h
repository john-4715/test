#ifndef __SPI_OLED_H__
#define __SPI_OLED_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FLASH_MAX_SIZE (4 * 1024 * 1024)
#define FLASH_PAGE_SIZE (64 * 1024)

// spi初始化
int SPI_Open(const char *device, int *fd);

int SPI_Write(int fd, uint8_t *TxBuf, int len);

int SPI_Read(int fd, uint8_t *RxBuf, int len);

// spi发送数据
int SPI_Transfer(int fd, const uint8_t *TxBuf, uint8_t *RxBuf, int len);

int SPI_LookBackTest(int fd);

int SPI_Close(int fd);

int flash_write(int device_fd, int write_filefd, unsigned int length);

int flash_read(int device_fd, int read_filefd, unsigned int length);
#endif