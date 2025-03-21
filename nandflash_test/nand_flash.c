
#include "nand_flash.h"

NAND_FLASH_INSTAN *creare_nandflash_instance()
{
	NAND_FLASH_INSTAN *pObj = (NAND_FLASH_INSTAN *)malloc(sizeof(NAND_FLASH_INSTAN *));
	if (pObj)
	{
		pObj->NandWrite = nand_write;
		pObj->NandRead = nand_read;
		pObj->NandErase = nand_erase;
	}
}

void release_nandflash_instance(NAND_FLASH_INSTAN *pObj)
{
	if (pObj)
	{
		pObj->NandWrite = NULL;
		pObj->NandRead = NULL;
		pObj->NandErase = NULL;
		free(pObj);
		pObj = NULL;
	}
}

int Flash_Read(int fd, off_t offset, unsigned int len, char *pBuf)
{
	int readLen = 0;

	if (offset != lseek(fd, offset, SEEK_SET))
	{
		perror("lseek");
		return -1;
	}
	readLen = read(fd, pBuf, len);
	if (readLen < 0)
	{
		return -1;
	}
	return readLen;
}

int Flash_Writebyte(int fd, off_t offset, char *pBuf)
{
	int writeLen = 0;
	unsigned char value;

	if (offset != lseek(fd, offset, SEEK_SET))
	{
		perror("lseek");
		return -1;
	}

	value = (unsigned char)atoi(pBuf);

	fprintf(stderr, "-->value = %u\n", value);
	writeLen = write(fd, (char *)&value, sizeof(unsigned char));
	if (writeLen < 0)
	{
		perror("write");
		return -1;
	}
	return writeLen;
}

unsigned long Read_device_info(int fd, const char *filepath, off_t offset, unsigned long len)
{
	char buf[BUF_SIZE];
	int ntotalLen = 0;
	int remains = len;
	int buff_size;
	int i;
	FILE *fp;
	if ((fp = fopen(filepath, "wb")) == NULL)
	{
		fprintf(stderr, "fopen %s fail\n", filepath);
		return -1;
	}
	while (1)
	{
		memset(buf, 0x00, BUF_SIZE);
		buff_size = (BUF_SIZE > remains) ? remains : BUF_SIZE;

		int readlen = Flash_Read(fd, offset, buff_size, buf);
		if (readlen > 0)
		{
			fwrite(buf, sizeof(char), readlen, fp);
			offset += readlen;
			ntotalLen += readlen;
			remains -= readlen;
			if (ntotalLen >= len)
			{
				break;
			}
		}
		else
		{
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);

	return ntotalLen;
}

static unsigned next_good_eraseblock(int fd, struct mtd_info_user *meminfo, unsigned block_offset)
{
	while (1)
	{
		loff_t offs;

		if (block_offset >= meminfo->size)
		{
			fprintf(stderr, "not enough space in MTD device");
			return block_offset; /* let the caller exit */
		}

		offs = block_offset;
		if (ioctl(fd, MEMGETBADBLOCK, &offs) == 0)
			return block_offset;

		/* ioctl returned 1 => "bad block" */
		fprintf(stderr, "Skipping bad block at 0x%08x\n", block_offset);
		block_offset += meminfo->erasesize;
	}
}

int nand_write(const char *device_name, const char *file_name, const int mtd_offset)
{
	mtd_info_t meminfo;
	unsigned int blockstart;
	unsigned int limit = 0;
	int cnt = -1;
	int size = 0;
	int ret = 0;
	int offset = mtd_offset;

	// fopen input file
	FILE *pf = fopen(file_name, "r");
	if (pf == NULL)
	{
		fprintf(stderr, "fopen %s failed!\n", file_name);
		return -1;
	}

	// open mtd device
	int fd = open(device_name, O_WRONLY);
	if (fd < 0)
	{
		fprintf(stderr, "open %s failed!\n", device_name);
		fclose(pf);
		return -1;
	}

	// get meminfo
	ret = ioctl(fd, MEMGETINFO, &meminfo);
	if (ret < 0)
	{
		fprintf(stderr, "get MEMGETINFO failed!\n");
		fclose(pf);
		close(fd);
		return -1;
	}

	limit = meminfo.size;

	// check offset page aligned
	if (offset & (meminfo.writesize - 1))
	{
		fprintf(stderr, "start address is not page aligned");
		fclose(pf);
		close(fd);
		return -1;
	}

	// malloc buffer for read
	char *tmp = (char *)malloc(meminfo.writesize);
	if (tmp == NULL)
	{
		fprintf(stderr, "malloc %d size buffer failed!\n", meminfo.writesize);
		fclose(pf);
		close(fd);
		return -1;
	}

	// if offset in a bad block, get next good block
	blockstart = offset & ~(meminfo.erasesize - 1);
	if (offset != blockstart)
	{
		unsigned int tmp;
		tmp = next_good_eraseblock(fd, &meminfo, blockstart);
		if (tmp != blockstart)
		{
			offset = tmp;
		}
	}

	while (offset < limit)
	{
		blockstart = offset & ~(meminfo.erasesize - 1);
		if (blockstart == offset)
		{
			offset = next_good_eraseblock(fd, &meminfo, blockstart);
			fprintf(stderr, "Writing at 0x%08x\n", offset);

			if (offset >= limit)
			{
				fprintf(stderr, "offset(%d) over limit(%d)\n", offset, limit);
				fclose(pf);
				close(fd);
				free(tmp);
				return -1;
			}
		}

		lseek(fd, offset, SEEK_SET);

		cnt = fread(tmp, 1, meminfo.writesize, pf);
		if (cnt == 0)
		{
			fprintf(stderr, "write ok!\n");
			break;
		}

		if (cnt < meminfo.writesize)
		{
			/* zero pad to end of write block */
			memset(tmp + cnt, 0, meminfo.writesize - cnt);
		}

		size = write(fd, tmp, meminfo.writesize);
		if (size != meminfo.writesize)
		{
			fprintf(stderr, "write err, need :%d, real :%d\n", meminfo.writesize, size);
			fclose(pf);
			close(fd);
			free(tmp);
			return -1;
		}

		offset += meminfo.writesize;

		if (cnt < meminfo.writesize)
		{
			fprintf(stderr, "write ok!\n");
			break;
		}
	}

	// free buf
	free(tmp);
	fclose(pf);
	close(fd);

	return 0; // test
}

unsigned long nand_read(const char *device_name, const char *file_name, const int offset, unsigned long nlen)
{
	int fd;

	if ((fd = open(device_name, O_SYNC | O_RDWR)) < 0)
	{
		perror("open");
		return -1;
	}
	unsigned long ntotlelen = Read_device_info(fd, file_name, offset, nlen);
	close(fd);
	return ntotlelen;
}

int nand_erase(const char *devicename, const int offset)
{
	int fd;
	int ret = 0;
	struct stat st;
	mtd_info_t meminfo;
	erase_info_t erase;

	// open mtd device
	fd = open(devicename, O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "open %s failed!\n", devicename);
		return -1;
	}

	// check is a char device
	ret = fstat(fd, &st);
	if (ret < 0)
	{
		fprintf(stderr, "fstat %s failed!\n", devicename);
		close(fd);
		return -1;
	}

	if (!S_ISCHR(st.st_mode))
	{
		fprintf(stderr, "%s: not a char device", devicename);
		close(fd);
		return -1;
	}

	// get meminfo
	ret = ioctl(fd, MEMGETINFO, &meminfo);
	if (ret < 0)
	{
		fprintf(stderr, "get MEMGETINFO failed!\n");
		close(fd);
		return -1;
	}

	erase.length = meminfo.erasesize;
	fprintf(stderr, "hejr---->size = %lu, erasesize = %u, writesize = %u, oobsize = %u\n", meminfo.size,
			meminfo.erasesize, meminfo.writesize, meminfo.oobsize);

	for (erase.start = offset; erase.start < offset + meminfo.size; erase.start += meminfo.erasesize)
	{
		loff_t bpos = erase.start;

		// check bad block
		ret = ioctl(fd, MEMGETBADBLOCK, &bpos);
		if (ret > 0)
		{
			fprintf(stderr, "mtd: not erasing bad block at 0x%08llx\n", bpos);
			continue; // Don't try to erase known factory-bad blocks.
		}

		if (ret < 0)
		{
			fprintf(stderr, "MEMGETBADBLOCK error");
			close(fd);
			return -1;
		}

		// erase
		if (ioctl(fd, MEMERASE, &erase) < 0)
		{
			fprintf(stderr, "mtd: erase failure at 0x%08llx\n", bpos);
			close(fd);
			return -1;
		}
	}

	fprintf(stderr, "hejr--->erase ok.\n");
	close(fd);
	return 0;
}
