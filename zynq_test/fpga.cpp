#include "fpga.h"
#include <stdio.h>

#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE - 1))

#define MEM_MAP_SIZE (0x10000)

int rd_reg_array[49] = {
	xPAA_RD_DATA_REG1,	xPAA_RD_DATA_REG2,	xPAA_RD_DATA_REG3,	xPAA_RD_DATA_REG4,	xPAA_RD_DATA_REG5,
	xPAA_RD_DATA_REG6,	xPAA_RD_DATA_REG7,	xPAA_RD_DATA_REG8,	xPAA_RD_DATA_REG9,	xPAA_RD_DATA_REG10,
	xPAA_RD_DATA_REG11, xPAA_RD_DATA_REG12, xPAA_RD_DATA_REG13, xPAA_RD_DATA_REG14, xPAA_RD_DATA_REG15,
	xPAA_RD_DATA_REG16, xPAA_RD_DATA_REG17, xPAA_RD_DATA_REG18, xPAA_RD_DATA_REG19, xPAA_RD_DATA_REG20,
	xPAA_RD_DATA_REG21, xPAA_RD_DATA_REG22, xPAA_RD_DATA_REG23, xPAA_RD_DATA_REG24, xPAA_RD_DATA_REG25,
	xPAA_RD_DATA_REG26, xPAA_RD_DATA_REG27, xPAA_RD_DATA_REG28, xPAA_RD_DATA_REG29, xPAA_RD_DATA_REG30,
	xPAA_RD_DATA_REG31, xPAA_RD_DATA_REG32, xPAA_RD_DATA_REG33, xPAA_RD_DATA_REG34, xPAA_RD_DATA_REG35,
	xPAA_RD_DATA_REG36, xPAA_RD_DATA_REG37, xPAA_RD_DATA_REG38, xPAA_RD_DATA_REG39, xPAA_RD_DATA_REG40,
	xPAA_RD_DATA_REG41, xPAA_RD_DATA_REG42, xPAA_RD_DATA_REG43, xPAA_RD_DATA_REG44, xPAA_RD_DATA_REG45,
	xPAA_RD_DATA_REG46, xPAA_RD_DATA_REG47, xPAA_RD_DATA_REG48, xPAA_RD_DATA_REG49

};

int wr_reg_array[5] = {xPAA_WR_DATA_REG1, xPAA_WR_DATA_REG2, xPAA_WR_DATA_REG3, xPAA_WR_DATA_REG4, xPAA_WR_DATA_REG5};

WR_REG_VALUE wr_reg_vale_array[5] = {{xPAA_WR_DATA_REG1, 0x1010},
									 {xPAA_WR_DATA_REG2, 0x2020},
									 {xPAA_WR_DATA_REG3, 0x3030},
									 {xPAA_WR_DATA_REG4, 0x4040},
									 {xPAA_WR_DATA_REG5, 0x5050}};

fpga::fpga()
{
	initFlg = 0;
	fpgaPgOffset = 0;
}

int fpga::fpgaInit(uint32_t BaseAddr)
{
	initFlg = 0;
	// 打开mem文件
	int fd = open("/dev/mem", O_RDWR | O_SYNC | O_DSYNC | O_RSYNC);
	if (fd == -1)
	{
		printf("open /dev/mem error!\n");
		return -1;
	}

	// uint32_t base = BaseAddr & PAGE_MASK;
	// fpgaPgOffset = BaseAddr & (~PAGE_MASK);
	// 地址映射
	fpgaMapBase = (volatile uint8_t *)mmap(NULL, MEM_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BaseAddr);
	if (fpgaMapBase == MAP_FAILED)
	{
		printf("mmap error!\n");
		close(fd);
		return -2;
	}
	// 关闭文件
	close(fd);

	initFlg = 1;

	return 0;
}

void fpga::fpgaWrite32(uint32_t Reg, uint32_t Data)
{
	if (initFlg == 0)
	{
		printf("fpga is not init!\n");
		return;
	}
	printf("write reg addr:0x%0x\n", Reg);
	*(volatile uint32_t *)(fpgaMapBase + Reg) = Data;
}

int fpga::fpgaRead32(uint32_t Reg)
{
	uint32_t Value;

	if (initFlg == 0)
	{
		printf("fpga is not init!\n");
		return -1;
	}

	Value = *(volatile uint32_t *)(fpgaMapBase + Reg);

	return Value;
}

void fpga::fpgaDeInit()
{
	initFlg = 0;
	munmap((void *)fpgaMapBase, PAGE_SIZE);
}

int main(int argc, char *argv[])
{

	// xMen xMem;
	fpga xFPGA;
	int Status, i = 0;

	// 传入物理地址
	Status = xFPGA.fpgaInit(VBASE_ADDRESS);

	if (Status != 0)
	{
		printf("fpga init error!\n");
	}
	printf("FPGA READ!\n");
	// 先读出初始数据
	for (i = 0; i < 49; i++)
	{
		printf("RD_DATA_REG0:%0x\n", xFPGA.fpgaRead32(rd_reg_array[i]));
	}

	printf("FPGA WRITE!\n");
	// 写数据
	for (i = 0; i < 5; i++)
	{
		xFPGA.fpgaWrite32(wr_reg_vale_array[i].reg, wr_reg_vale_array[i].value);
	}

	// 再次读出数据
	for (i = 0; i < 5; i++)
	{
		printf("WR_DATA_REG0:%0x\n", xFPGA.fpgaRead32(wr_reg_array[i]));
	}
	// 测试完成
	xFPGA.fpgaDeInit();

	return 0;
}
