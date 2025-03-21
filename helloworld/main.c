#include <stdio.h>
#include <string.h>

int main()
{
	unsigned char stationNo = 0x85;
	printf("stationNo=0x%x\n", stationNo);
	stationNo = ~ stationNo;
	printf("stationNo=0x%x\n", stationNo);
	printf("hello world!!!\n");
	return 0;
}
