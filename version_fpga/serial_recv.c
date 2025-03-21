
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FPGA_DEV_NAME "/dev/mem"

static unsigned int vbase_address = 0;

#define xPAA_RD_DATA_REG1 0x0404
#define xPAA_RD_DATA_REG2 0x0408
#define xPAA_RD_DATA_REG3 0x040c
#define xPAA_RD_DATA_REG4 0x0410
#define xPAA_RD_DATA_REG5 0x0414

#define MEM_MAP_SIZE (0x10000)

#define FPGA_CTRL_SET_ADDR(x, data) (*(unsigned int *)(vbase_address + x) = data)
#define FPGA_CTRL_GET_ADDR(x) (*(unsigned int *)(vbase_address + x))

#define SEND_DATA_LEN 256 // 280 //256

/*
 *   fpga_dev_init
 */
int fpga_dev_init(void)
{

	int g_memfd = -1;

	if ((g_memfd = open("/dev/mem", O_RDWR | O_SYNC | O_DSYNC | O_RSYNC)) < 0)
	{
		return -1;
	}
	vbase_address = (unsigned int)mmap(0, MEM_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_memfd, 0x43c00000);
	printf("vbase_address = %u\n", vbase_address);
	close(g_memfd);

	return 0;
}

/*
 *   fpga_dev_uninit
 */
void fpga_dev_uninit(void) { munmap((void *)vbase_address, MEM_MAP_SIZE); }

int main(int argc, char *argv[])
{
	int uidata = 0;
	int reg = 0;

	int rd_reg_array[5] = {xPAA_RD_DATA_REG1, xPAA_RD_DATA_REG2, xPAA_RD_DATA_REG3, xPAA_RD_DATA_REG4,
						   xPAA_RD_DATA_REG5};

	fpga_dev_init();
	// printf("请输入串口编号：");
	// scanf("%x", &reg);

	printf("串口接收数据为：");
	while (1)
	{
		for (int i = 0; i < 5; i++)
		{
			uidata = FPGA_CTRL_GET_ADDR(rd_reg_array[i]);
			printf("%0x ", uidata);
		}
		printf("\n");
		usleep(500000);
	}

	fpga_dev_uninit();
}
