
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
#define xPAA_RD_DATA_REG3 0x8008
#define xPAA_RD_DATA_REG4 0x800C
#define xPAA_RD_DATA_REG5 0x8010
#define xPAA_RD_DATA_REG6 0x8014
#define xPAA_RD_DATA_REG7 0x8018
#define xPAA_RD_DATA_REG8 0x801C
#define xPAA_RD_DATA_REG9 0x8020
#define xPAA_RD_DATA_REG10 0x8024
#define xPAA_RD_DATA_REG11 0x8028
#define xPAA_RD_DATA_REG12 0x802C
#define xPAA_RD_DATA_REG13 0x8030
#define xPAA_RD_DATA_REG14 0x8034
#define xPAA_RD_DATA_REG15 0x8038
#define xPAA_RD_DATA_REG16 0x803C
#define xPAA_RD_DATA_REG17 0x8040
#define xPAA_RD_DATA_REG18 0x9000
#define xPAA_RD_DATA_REG19 0x9004
#define xPAA_RD_DATA_REG20 0x9008
#define xPAA_RD_DATA_REG21 0x900C
#define xPAA_RD_DATA_REG22 0x9010
#define xPAA_RD_DATA_REG23 0x9014
#define xPAA_RD_DATA_REG24 0x9018
#define xPAA_RD_DATA_REG25 0x901C
#define xPAA_RD_DATA_REG26 0x9020
#define xPAA_RD_DATA_REG27 0x9024
#define xPAA_RD_DATA_REG28 0x9028
#define xPAA_RD_DATA_REG29 0x902C
#define xPAA_RD_DATA_REG30 0x9030
#define xPAA_RD_DATA_REG31 0x9034
#define xPAA_RD_DATA_REG32 0x9038
#define xPAA_RD_DATA_REG33 0x903C
#define xPAA_RD_DATA_REG34 0x9040
#define xPAA_RD_DATA_REG35 0x9044
#define xPAA_RD_DATA_REG36 0x9048
#define xPAA_RD_DATA_REG37 0x904C
#define xPAA_RD_DATA_REG38 0x9050
#define xPAA_RD_DATA_REG39 0x9054
#define xPAA_RD_DATA_REG40 0x9058
#define xPAA_RD_DATA_REG41 0x905C
#define xPAA_RD_DATA_REG42 0x9060
#define xPAA_RD_DATA_REG43 0x9064
#define xPAA_RD_DATA_REG44 0x9068
#define xPAA_RD_DATA_REG45 0x906C
#define xPAA_RD_DATA_REG46 0x9070
#define xPAA_RD_DATA_REG47 0x9074
#define xPAA_RD_DATA_REG48 0x9078
#define xPAA_RD_DATA_REG49 0x907C

#define xPAA_WR_DATA_REG1 0x7000
#define xPAA_WR_DATA_REG2 0x7004
#define xPAA_WR_DATA_REG3 0x7008
#define xPAA_WR_DATA_REG4 0x700C
#define xPAA_WR_DATA_REG5 0x7010

#define Flag_Addr 0x8004
#define Data_Addr 0x8008
#define INT_Flag_Addr 0x800c
#define INT_Data_Addr 0x8010
#define START_FLAG 0x8024
#define W_DATA 0x8020
#define MEM_MAP_SIZE (0x10000)

#define FPGA_CTRL_SET_ADDR(x, data) (*(unsigned int *)(vbase_address + x) = data)
#define FPGA_CTRL_GET_ADDR(x) (*(unsigned int *)(vbase_address + x))

#define SEND_DATA_LEN 256 // 280 //256

typedef struct _WR_REG_VALUE
{
	int reg;
	int value;
} WR_REG_VALUE;
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

	// int rd_reg_array[49] = {
	// 	xPAA_RD_DATA_REG1,	xPAA_RD_DATA_REG2,	xPAA_RD_DATA_REG3,	xPAA_RD_DATA_REG4,	xPAA_RD_DATA_REG5,
	// 	xPAA_RD_DATA_REG6,	xPAA_RD_DATA_REG7,	xPAA_RD_DATA_REG8,	xPAA_RD_DATA_REG9,	xPAA_RD_DATA_REG10,
	// 	xPAA_RD_DATA_REG11, xPAA_RD_DATA_REG12, xPAA_RD_DATA_REG13, xPAA_RD_DATA_REG14, xPAA_RD_DATA_REG15,
	// 	xPAA_RD_DATA_REG16, xPAA_RD_DATA_REG17, xPAA_RD_DATA_REG18, xPAA_RD_DATA_REG19, xPAA_RD_DATA_REG20,
	// 	xPAA_RD_DATA_REG21, xPAA_RD_DATA_REG22, xPAA_RD_DATA_REG23, xPAA_RD_DATA_REG24, xPAA_RD_DATA_REG25,
	// 	xPAA_RD_DATA_REG26, xPAA_RD_DATA_REG27, xPAA_RD_DATA_REG28, xPAA_RD_DATA_REG29, xPAA_RD_DATA_REG30,
	// 	xPAA_RD_DATA_REG31, xPAA_RD_DATA_REG32, xPAA_RD_DATA_REG33, xPAA_RD_DATA_REG34, xPAA_RD_DATA_REG35,
	// 	xPAA_RD_DATA_REG36, xPAA_RD_DATA_REG37, xPAA_RD_DATA_REG38, xPAA_RD_DATA_REG39, xPAA_RD_DATA_REG40,
	// 	xPAA_RD_DATA_REG41, xPAA_RD_DATA_REG42, xPAA_RD_DATA_REG43, xPAA_RD_DATA_REG44, xPAA_RD_DATA_REG45,
	// 	xPAA_RD_DATA_REG46, xPAA_RD_DATA_REG47, xPAA_RD_DATA_REG48, xPAA_RD_DATA_REG49

	// };

	// int wr_reg_array[5] = {xPAA_WR_DATA_REG1, xPAA_WR_DATA_REG2, xPAA_WR_DATA_REG3, xPAA_WR_DATA_REG4,
	// 					   xPAA_WR_DATA_REG5};

	// WR_REG_VALUE wr_reg_vale_array[5] = {{xPAA_WR_DATA_REG1, 0x1010},
	// 									 {xPAA_WR_DATA_REG2, 0x2020},
	// 									 {xPAA_WR_DATA_REG3, 0x3030},
	// 									 {xPAA_WR_DATA_REG4, 0x4040},
	// 									 {xPAA_WR_DATA_REG5, 0x5050}};

	fpga_dev_init();

	int reg;

	while (1)
	{
		printf("input regs=");
		scanf("%x", &reg);
		uidata = FPGA_CTRL_GET_ADDR(reg);
		printf("reg = 0x%x, value = 0x%x\n", reg, uidata);
	}

	fpga_dev_uninit();
}
