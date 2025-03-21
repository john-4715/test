
#ifndef FPGA_H
#define FPGA_H

#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

// Write Data Reg Define
#define xPAA_WR_DATA_REG1 0x7000
#define xPAA_WR_DATA_REG2 0x7004
#define xPAA_WR_DATA_REG3 0x7008
#define xPAA_WR_DATA_REG4 0x700C
#define xPAA_WR_DATA_REG5 0x7010

// Write Data Control Reg
#define xPAA_WR_CTRL_REG 32

// Read Data Reg Define
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

typedef struct _WR_REG_VALUE
{
	int reg;
	int value;
} WR_REG_VALUE;

// Read Data Control Reg
#define VBASE_ADDRESS 0X83C00000

class fpga
{
public:
	fpga();

	int fpgaInit(uint32_t BaseAddr);
	void fpgaDeInit();
	void fpgaWrite32(uint32_t Reg, uint32_t Data);
	int fpgaRead32(uint32_t Reg);

private:
	uint8_t initFlg;
	uint32_t fpgaPgOffset;
	volatile uint8_t *fpgaMapBase;
};

#endif // FPGA_H
