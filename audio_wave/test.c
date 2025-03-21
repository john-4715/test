#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void init_data1(char *tx_buf, size_t tx_buf_size)
{

	int ft = 10;		  /* 周期 hz */
	int fs = (64 * 1024); /* 采样频率*/
	short input_data[2000];
	int offset;
	static int i = 0;
	int len = i;

	for (int idx = 0; idx < tx_buf_size; i++, idx++)
	{
		input_data[idx] = 5 * cos(2 * 3.1415926 * ft * i / fs);
		printf("input_data[%d]=%d\n", i, input_data[idx]);
		// memcpy(&tx_buf[4 * idx], &input_data[idx], 4);
		memcpy(&tx_buf[4 * idx], &idx, 4);
	}

	return;
}

int main(void)
{
#if 1
	int radian; // 10倍弧度
	double y;	// y为y轴，值为-1到+1，步长为0.1；
	int x;		// 用62列值代表x轴；62来源于2Pi*10约等于2*3.1*10；

	for (y = 1; y >= -1; y -= 0.1)
	{
		radian = asin(y) * 10;

		for (x = 0; x <= 62; x++)
		{
			if ((x == radian) || (x == (31 - radian) || (x == radian + 61))) // 反余弦函数值域[0，pi]
				printf("*", y);
			// else if (radian == 0)
			// 	printf("-");
			else
				printf(" ");
		}
		printf("\n");
	}
#else
	char tx_buf[8000];
	init_data1(tx_buf, 2000);
#endif
	return 0;
}
