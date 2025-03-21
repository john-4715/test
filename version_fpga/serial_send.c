
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

#define xPAA_RD_DATA_REG1 0x8000
#define xPAA_RD_DATA_REG2 0x8004

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

int send_data_integer(int reg)
{
	for (int i = 0; i < 100; i++)
	{
		printf("pstr[i]=0x%02x\n", i);
		FPGA_CTRL_SET_ADDR(reg, i);
		printf("interrupt start\n");
		usleep(200);
		FPGA_CTRL_SET_ADDR(0x1500, 1);
		usleep(2000);
		FPGA_CTRL_SET_ADDR(0x1500, 0);
		usleep(200);
		printf("interrupt stop\n");
	}
	return 0;
}

int send_data(int reg, char *pstr, int length)
{
	for (int i = 0; i < length; i++)
	{
		printf("pstr[i]=0x%02x\n", pstr[i]);
		FPGA_CTRL_SET_ADDR(reg, (int)pstr[i]);
		usleep(200);
		FPGA_CTRL_SET_ADDR(0x1500, 1);
		usleep(2000);
		FPGA_CTRL_SET_ADDR(0x1500, 0);
		usleep(200);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int reg;
	fpga_dev_init();

	while (1)
	{
		printf("请输入串口编号：");
		scanf("%x", &reg);
#if 0
        char str[1024] = {0};
		printf("请输入一组字符串：");
		scanf("%s", str);
		send_data(reg, str, strlen(str));
#else
		send_data_integer(reg);
#endif
	}

	fpga_dev_uninit();
}
