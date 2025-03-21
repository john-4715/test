
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

#define MEM_MAP_SIZE (0x2000)

#define FPGA_CTRL_SET_ADDR(x, data) (*(unsigned int *)(vbase_address + x) = data)
#define FPGA_CTRL_GET_ADDR(x) (*(unsigned int *)(vbase_address + x))
#define FPGA_CTRL_MEMCPY(buffer, len) memcpy(vbase_address, buffer, len)

#define PointMax 64
#define PI 3.1415926
unsigned int sinData[PointMax] = {0};

typedef struct _AUDIO_WAVE
{
	int frameno;
	int len;
	short *audio_data;
} AUDIO_WAVE;
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
	vbase_address = (unsigned int)mmap(0, MEM_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_memfd, 0x80000000);
	printf("vbase_address = %u\n", vbase_address);
	close(g_memfd);

	return 0;
}

/*
 *   fpga_dev_uninit
 */
void fpga_dev_uninit(void) { munmap((void *)vbase_address, MEM_MAP_SIZE); }

// point 一个周期内采样的点数
// 生成一个周期正弦波 水平线为32  最大值为64  最小值为0
void get_sin_tab(unsigned int point)
{
	int i = 0, j = 0;
	float hd = 0.0; // 弧度
	float fz = 0.0; // 峰值
	short tem = 0;
	j = point / 2;
	hd = PI / j;

	AUDIO_WAVE audio_wave = {0};
	audio_wave.frameno = 0x55AA;
	audio_wave.len = point;

	audio_wave.audio_data = (short *)malloc(point * sizeof(short));
	if (NULL == audio_wave.audio_data)
	{
		return;
	}
	for (i = 0; i < point; i++)
	{
		fz = j * sin(hd * i);
		tem = (int)fz;
		audio_wave.audio_data[i] = tem;
		printf("%d,", tem);
	}
	printf("\r\n");
	int total_len = 2 * sizeof(int) + point * sizeof(short);

	FPGA_CTRL_MEMCPY(&audio_wave, total_len);
	free(audio_wave.audio_data);
}

int main(int argc, char *argv[])
{
	int point = 0;
	fpga_dev_init();

	printf("输入一个周期内采样的点数：");
	scanf("%d", &point);
	if (point > 8192)
	{
		printf("Sampling points cannot exceed 8192.\n");
		return -1;
	}
	get_sin_tab(point);

	fpga_dev_uninit();
	return 0;
}
