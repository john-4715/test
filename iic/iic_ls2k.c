#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// #define i2c_DEBUG
int higher2lower(char c);
int htoi(char s[]);
#define MSB_0_7_OF_U32(DATA) ((DATA >> 24) & 0xFF)
#define MSB_8_15_OF_U32(DATA) ((DATA >> 16) & 0xFF)
#define MSB_16_23_OF_U32(DATA) ((DATA >> 8) & 0xFF)
#define MSB_24_31_OF_U32(DATA) ((DATA >> 0) & 0xFF)
#define REG_VAL_32_BIT(VAL_8_BIT_0, VAL_8_BIT_1, VAL_8_BIT_2, VAL_8_BIT_3)                                             \
	((VAL_8_BIT_0 << 24) | (VAL_8_BIT_1 << 16) | (VAL_8_BIT_2 << 8) | VAL_8_BIT_3)
int i2c_nms_access_7b(unsigned int i2c_adaptor, unsigned int slave_address, unsigned int *reg_address,
					  unsigned int *reg_val, unsigned int rw_flag)
{
	int fd, i, j, ret;
	char *errorinfo;
	struct i2c_rdwr_ioctl_data work_queue;
	unsigned int byte_count, format_nms3210, reg_num;
	unsigned char val;

	switch (i2c_adaptor)
	{
	case 0:
		fd = open("/dev/i2c-0", O_RDWR);
		break;

	case 1:
		fd = open("/dev/i2c-1", O_RDWR);
		break;

	case 2:
		fd = open("/dev/i2c-2", O_RDWR);
		break;

	case 3:
		fd = open("/dev/i2c-3", O_RDWR);
		break;

	default:
		fd = open("/dev/i2c-1", O_RDWR);
		break;
	}
	if (!fd)
	{
		printf("Error on opening the device file\n");
		return 0;
	}

	// i2c configuration
	ioctl(fd, I2C_TIMEOUT, 2);
	ioctl(fd, I2C_RETRIES, 1);

	// i2c operation
	if (rw_flag)
	{
		work_queue.nmsgs = 2;
		work_queue.msgs = (struct i2c_msg *)malloc(work_queue.nmsgs * sizeof(struct i2c_msg));

		if (!work_queue.msgs)
		{
			printf("memory alloc error\n");
			close(fd);
			return 0;
		}

		reg_num = 1;
		format_nms3210 = (rw_flag << 31) | ((reg_num - 1) << 22) | (*reg_address >> 2);
		//		printf("format_nms3210 : %#x\n", format_nms3210);
		(work_queue.msgs[0]).len = 4;
		(work_queue.msgs[0]).addr = slave_address;
		(work_queue.msgs[0]).flags = 0; // w
		(work_queue.msgs[0]).buf = (unsigned char *)malloc(4);
		//		(work_queue.msgs[0]).buf[0] = 0xD8;
		(work_queue.msgs[0]).buf[0] = MSB_0_7_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[1] = MSB_8_15_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[2] = MSB_16_23_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[3] = MSB_24_31_OF_U32(format_nms3210);
		//		(work_queue.msgs[0]).buf[3] = MSB_0_7_OF_U32(format_nms3210);
		//		(work_queue.msgs[0]).buf[2] = MSB_8_15_OF_U32(format_nms3210);
		//		(work_queue.msgs[0]).buf[1] = MSB_16_23_OF_U32(format_nms3210);
		//		(work_queue.msgs[0]).buf[0] = MSB_24_31_OF_U32(format_nms3210);
		//	(work_queue.msgs[0]).buf[0] = MSB_16_23_OF_U32(reg_address);
		//	(work_queue.msgs[0]).buf[1] = MSB_24_31_OF_U32(reg_address);

		(work_queue.msgs[1]).len = 4;
		(work_queue.msgs[1]).flags = I2C_M_RD; // R
		(work_queue.msgs[1]).addr = slave_address;
		(work_queue.msgs[1]).buf = (unsigned char *)malloc(4);
#ifdef i2c_DEBUG
		for (j = 0; j < 4; j++)
		{
			printf("work_queue.msgs[0].buf[%d] : %#x\n", j, work_queue.msgs[0].buf[j]);
		}
#endif
		ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
		errorinfo = strerror(errno);
		printf("errorinfo_read : %s\n", errorinfo);
#ifdef i2c_DEBUG
		for (j = 0; j < 4; j++)
		{
			printf("work_queue.msgs[1].buf[%d] : %#x\n", j, work_queue.msgs[1].buf[j]);
		}
#endif
		*reg_val = REG_VAL_32_BIT(work_queue.msgs[1].buf[0], work_queue.msgs[1].buf[1], work_queue.msgs[1].buf[2],
								  work_queue.msgs[1].buf[3]);
		if (ret < 0)
			printf("error code : %d\n", ret);
		else
			printf("slave_addr : %#x  reg_addr : %#x  reg_val : %#x\n", slave_address, *reg_address, *reg_val);
		free(work_queue.msgs);
	}
	else
	{
		work_queue.nmsgs = 1;
		work_queue.msgs = (struct i2c_msg *)malloc(work_queue.nmsgs * sizeof(struct i2c_msg));

		if (!work_queue.msgs)
		{
			printf("memory alloc error\n");
			close(fd);
			return 0;
		}
		reg_num = 1;
		format_nms3210 = (rw_flag << 31) | ((reg_num - 1) << 22) | (*reg_address >> 2);
		//	printf("format_nms3210 : %#x\n", format_nms3210);
		(work_queue.msgs[0]).len = 8; // reg_addr 4 + reg_val 4
		(work_queue.msgs[0]).addr = slave_address;
		(work_queue.msgs[0]).flags = 0; // w
		(work_queue.msgs[0]).buf = (unsigned char *)malloc(8);
		(work_queue.msgs[0]).buf[0] = MSB_0_7_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[1] = MSB_8_15_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[2] = MSB_16_23_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[3] = MSB_24_31_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[4] = MSB_0_7_OF_U32(*reg_val);
		(work_queue.msgs[0]).buf[5] = MSB_8_15_OF_U32(*reg_val);
		(work_queue.msgs[0]).buf[6] = MSB_16_23_OF_U32(*reg_val);
		(work_queue.msgs[0]).buf[7] = MSB_24_31_OF_U32(*reg_val);

		ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
		errorinfo = strerror(errno);
		printf("errorinfo_write : %s\n", errorinfo);
		//		reg_val = REG_VAL_32_BIT(work_queue.msgs[1].buf[0], work_queue.msgs[1].buf[1], \
						work_queue.msgs[1].buf[2], work_queue.msgs[1].buf[3]);
		if (ret < 0)
			printf("error code : %d\n", ret);

		free(work_queue.msgs);
	}

	close(fd);
	return ret;
}
int i2c_nms_access_10b(unsigned int i2c_adaptor, unsigned int slave_address, unsigned int *reg_address,
					   unsigned int *reg_val, unsigned int rw_flag)
{
	int fd, i, j, ret;
	char *errorinfo;
	struct i2c_rdwr_ioctl_data work_queue;
	unsigned int byte_count, format_nms3210, reg_num;
	unsigned char val;

	switch (i2c_adaptor)
	{
	case 0:
		fd = open("/dev/i2c-0", O_RDWR);
		break;

	case 1:
		fd = open("/dev/i2c-1", O_RDWR);
		break;

	case 2:
		fd = open("/dev/i2c-2", O_RDWR);
		break;

	case 3:
		fd = open("/dev/i2c-3", O_RDWR);
		break;

	default:
		fd = open("/dev/i2c-1", O_RDWR);
		break;
	}
	if (!fd)
	{
		printf("Error on opening the device file\n");
		return 0;
	}

	// i2c configuration
	ioctl(fd, I2C_TIMEOUT, 2);
	ioctl(fd, I2C_RETRIES, 1);

	// i2c operation
	if (rw_flag)
	{
		work_queue.nmsgs = 2;
		work_queue.msgs = (struct i2c_msg *)malloc(work_queue.nmsgs * sizeof(struct i2c_msg));

		if (!work_queue.msgs)
		{
			printf("memory alloc error\n");
			close(fd);
			return 0;
		}

		reg_num = 1;
		format_nms3210 = (rw_flag << 31) | ((reg_num - 1) << 22) | (*reg_address >> 2);
		//		printf("format_nms3210 : %#x\n", format_nms3210);
		(work_queue.msgs[0]).len = 5;
		(work_queue.msgs[0]).addr = 0x7B;
		(work_queue.msgs[0]).flags = 0; // w
		(work_queue.msgs[0]).buf = (unsigned char *)malloc(5);
		(work_queue.msgs[0]).buf[0] = slave_address;
		(work_queue.msgs[0]).buf[1] = MSB_0_7_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[2] = MSB_8_15_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[3] = MSB_16_23_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[4] = MSB_24_31_OF_U32(format_nms3210);

		(work_queue.msgs[1]).len = 4;
		(work_queue.msgs[1]).flags = I2C_M_RD; // R
		(work_queue.msgs[1]).addr = 0x7B;
		(work_queue.msgs[1]).buf = (unsigned char *)malloc(4);
#ifdef i2c_DEBUG
		for (j = 0; j < 4; j++)
		{
			printf("work_queue.msgs[0].buf[%d] : %#x\n", j, work_queue.msgs[0].buf[j]);
		}
#endif
		ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
		errorinfo = strerror(errno);
		printf("errorinfo_read : %s\n", errorinfo);
#ifdef i2c_DEBUG
		for (j = 0; j < 4; j++)
		{
			printf("work_queue.msgs[1].buf[%d] : %#x\n", j, work_queue.msgs[1].buf[j]);
		}
#endif
		*reg_val = REG_VAL_32_BIT(work_queue.msgs[1].buf[0], work_queue.msgs[1].buf[1], work_queue.msgs[1].buf[2],
								  work_queue.msgs[1].buf[3]);
		if (ret < 0)
			printf("error code : %d\n", ret);
		else
			printf("slave_addr : %#x  reg_addr : %#x  reg_val : %#x\n", slave_address, *reg_address, *reg_val);
		free(work_queue.msgs);
	}
	else
	{
		work_queue.nmsgs = 1;
		work_queue.msgs = (struct i2c_msg *)malloc(work_queue.nmsgs * sizeof(struct i2c_msg));

		if (!work_queue.msgs)
		{
			printf("memory alloc error\n");
			close(fd);
			return 0;
		}
		reg_num = 1;
		format_nms3210 = (rw_flag << 31) | ((reg_num - 1) << 22) | (*reg_address >> 2);
		//	printf("format_nms3210 : %#x\n", format_nms3210);
		(work_queue.msgs[0]).len = 9; // 0x7B 1 + reg_addr 4 + reg_val 4
		(work_queue.msgs[0]).addr = 0x7B;
		(work_queue.msgs[0]).flags = 0; // w
		(work_queue.msgs[0]).buf = (unsigned char *)malloc(9);
		(work_queue.msgs[0]).buf[0] = slave_address;
		(work_queue.msgs[0]).buf[1] = MSB_0_7_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[2] = MSB_8_15_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[3] = MSB_16_23_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[4] = MSB_24_31_OF_U32(format_nms3210);
		(work_queue.msgs[0]).buf[5] = MSB_0_7_OF_U32(*reg_val);
		(work_queue.msgs[0]).buf[6] = MSB_8_15_OF_U32(*reg_val);
		(work_queue.msgs[0]).buf[7] = MSB_16_23_OF_U32(*reg_val);
		(work_queue.msgs[0]).buf[8] = MSB_24_31_OF_U32(*reg_val);

		ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
		errorinfo = strerror(errno);
		printf("errorinfo_write : %s\n", errorinfo);

		if (ret < 0)
			printf("error code : %d\n", ret);

		free(work_queue.msgs);
	}

	close(fd);
	return ret;
}
int i2c_pca9848_access(unsigned int i2c_adaptor, unsigned int slave_address, unsigned int *reg_val,
					   unsigned int rw_flag)
{
	int fd, i, j, ret;
	char *errorinfo;
	struct i2c_rdwr_ioctl_data work_queue;
	unsigned int byte_count, format_nms3210, reg_num;
	unsigned char val;

	switch (i2c_adaptor)
	{
	case 0:
		fd = open("/dev/i2c-0", O_RDWR);
		break;

	case 1:
		fd = open("/dev/i2c-1", O_RDWR);
		break;

	case 2:
		fd = open("/dev/i2c-2", O_RDWR);
		break;

	case 3:
		fd = open("/dev/i2c-3", O_RDWR);
		break;

	default:
		fd = open("/dev/i2c-1", O_RDWR);
		break;
	}
	if (!fd)
	{
		printf("Error on opening the device file\n");
		return 0;
	}

	// i2c configuration
	ioctl(fd, I2C_TIMEOUT, 2);
	ioctl(fd, I2C_RETRIES, 1);

	// i2c operation
	if (rw_flag)
	{
		work_queue.nmsgs = 1;
		work_queue.msgs = (struct i2c_msg *)malloc(work_queue.nmsgs * sizeof(struct i2c_msg));

		if (!work_queue.msgs)
		{
			printf("memory alloc error\n");
			close(fd);
			return 0;
		}

		(work_queue.msgs[0]).len = 1;
		(work_queue.msgs[0]).addr = slave_address;
		(work_queue.msgs[0]).flags = 1; // R
		(work_queue.msgs[0]).buf = (unsigned char *)malloc(1);

		ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
		errorinfo = strerror(errno);
		printf("errorinfo_read : %s\n", errorinfo);

		*reg_val = work_queue.msgs[0].buf[0];
		if (ret < 0)
			printf("error code : %d\n", ret);
		else
			printf("slave_addr : %#x  reg_val : %#x\n", slave_address, *reg_val);
		free(work_queue.msgs);
	}
	else
	{
		work_queue.nmsgs = 1;
		work_queue.msgs = (struct i2c_msg *)malloc(work_queue.nmsgs * sizeof(struct i2c_msg));

		if (!work_queue.msgs)
		{
			printf("memory alloc error\n");
			close(fd);
			return 0;
		}
		(work_queue.msgs[0]).len = 1; // reg_addr 4 + reg_val 4
		(work_queue.msgs[0]).addr = slave_address;
		(work_queue.msgs[0]).flags = 0; // w
		(work_queue.msgs[0]).buf = (unsigned char *)malloc(1);
		(work_queue.msgs[0]).buf[0] = *reg_val;

		ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
		errorinfo = strerror(errno);
		printf("errorinfo_write : %s\n", errorinfo);
		if (ret < 0)
			printf("error code : %d\n", ret);

		free(work_queue.msgs);
	}

	close(fd);
	return ret;
}

int main(int argc, char *argv[])
{
	int ret;
	char *errorinfo;
	unsigned int i2c_adaptor, i2c_channel_no, slave_address, reg_address, byte_count, reg_val, format_nms3210, reg_num,
		rw_flag;
	unsigned char val;

	unsigned int i2c_pca9848_slave_addr, i2c_pca9848_reg_val, i2c_pca9848_rw_flag;
	//	access_tran access;
	i2c_adaptor = htoi(argv[1]);
	i2c_channel_no = htoi(argv[2]);
	slave_address = htoi(argv[3]);
	reg_address = htoi(argv[4]);
	reg_val = htoi(argv[5]);
	rw_flag = htoi(argv[6]);
	printf("i2c_channel_no : %#x slave_addr : %#x reg_addr : %#x reg_val : %#x rw_flag : %#x\n", i2c_channel_no,
		   slave_address, reg_address, reg_val, rw_flag);
	/* Step 1
	 * Access pca9848 to choose which i2c-slave-device to access.
	 *
	 * For this LOONGSON-SW, the iic adaptor is fixed 1.
	 *
	 * There are 8 channels in pac9848 and the assignment is as follows:
	 * channel-no : device-name : i2c-addr
	 * channel-0  : nms1800A    : 0x58;
	 * channel-1  : nms1800B    : 0x58;
	 * channel-2  : nms1800C    : 0x58;
	 * channel-3  : ctc5160     : 0xxx;(unknown)
	 * channel-4  : tsi721      : 0xxx;(unknown)
	 * channel-5  : at24c64an   : 0xA0,
	 *              tmp75-0     : 0xE0,
	 *              tmp75-1     : 0xE2,
	 *              tmp75-2     : 0xE4,
	 *              tmp75-4     : 0xE6;
	 * channel-6  : ipmb        : 0xxx;(unknown)
	 * channle-7  : ad7997      : 0x40;
	 *
	 * Choose channel operation:
	 * i2c_pca9848_reg_val = 0x1 << channel-no;
	 * e.g.
	 * access nms1800A : i2c_pca9848_reg_val = 0x1 << 0;
	 * access ad7997   : i2c_pca9848_reg_val = 0x1 << 7;
	 */
	i2c_pca9848_slave_addr = 0x73;
	i2c_pca9848_reg_val = 0x1 << i2c_channel_no;
	i2c_pca9848_rw_flag = 0; // write
	i2c_pca9848_access(i2c_adaptor, i2c_pca9848_slave_addr, &i2c_pca9848_reg_val, i2c_pca9848_rw_flag);

	/* Step 2
	 * Access i2c device
	 */
	switch (i2c_channel_no)
	{
	case 0:
		//	i2c_nms_access_7b(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		i2c_nms_access_10b(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;

	case 1:
		//	i2c_nms_access_7b(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		i2c_nms_access_10b(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;

	case 2:
		//	i2c_nms_access_7b(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		i2c_nms_access_10b(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;

	case 3:
		//	i2c_ctc_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;

	case 4:
		//	i2c_tsi721_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;

	case 5:
		switch (slave_address)
		{
		case 0xA0:
			//	i2c_at24c64_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
			break;

		case 0xE0:
			//	i2c_tmp75_0_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
			break;

		case 0xE1:
			//	i2c_tmp75_1_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
			break;

		case 0xE2:
			//	i2c_tmp75_2_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
			break;

		case 0xE3:
			//	i2c_tmp75_3_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
			break;

		default:
			//	i2c_tmp75_0_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
			break;
		}
		break;

	case 6:
		//	i2c_ipmb_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;

	case 7:
		//	i2c_ad7997_access(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;

	default:
		i2c_nms_access_10b(i2c_adaptor, slave_address, &reg_address, &reg_val, rw_flag);
		break;
	}
}
int higher2lower(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 'a' - 'A';
	else
		return c;
}

int htoi(char s[])
{
	int i;
	int n = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'x'))
		i = 2;
	else
		i = 0;
	for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >= 'A' && s[i] <= 'F'); ++i)
	{
		if (higher2lower(s[i] > '9'))
		{
			n = 16 * n + (10 + higher2lower(s[i]) - 'a');
		}
		else
			n = 16 * n + (higher2lower(s[i] - '0'));
	}
	return n;
}
