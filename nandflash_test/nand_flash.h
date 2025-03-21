
#ifndef _TEST_NANDFLASH_H_
#define _TEST_NANDFLASH_H_

#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 256 // 缓冲区大小

// 回调函数
typedef int (*fptrNand_Write)(const char *device_name, const char *file_name, const int mtd_offset);
typedef unsigned long (*fptrNand_Read)(const char *device_name, const char *file_name, const int offset,
									   unsigned long nlen);
typedef int (*fptrNand_Erase)(const char *devicename, const int offset);

typedef struct _NAND_FLASH_INSTAN
{
	fptrNand_Write NandWrite;
	fptrNand_Read NandRead;
	fptrNand_Erase NandErase;
} NAND_FLASH_INSTAN;

NAND_FLASH_INSTAN *creare_nandflash_instance();

void release_nandflash_instance();

int Flash_Read(int fd, off_t offset, unsigned int len, char *pBuf);

int Flash_Writebyte(int fd, off_t offset, char *pBuf);

unsigned long Read_device_info(int fd, const char *filepath, off_t offset, unsigned long len);

static unsigned next_good_eraseblock(int fd, struct mtd_info_user *meminfo, unsigned block_offset);

int nand_write(const char *device_name, const char *file_name, const int mtd_offset);

unsigned long nand_read(const char *device_name, const char *file_name, const int offset, unsigned long nlen);

int nand_erase(const char *devicename, const int offset);

#endif
