#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "audio_data.h"
#include "clk.h"

int g_mclk_gpio_array[5] = {7, 4, 4, 1, 1};
int g_lrck_gpio_array[5] = {22, 3, 3, 0, 0};
int g_sclk_gpio_array[5] = {23, 5, 5, 2, 2};
int g_sdin_gpio_array[5] = {12, 13, 14, 15, 16};

AUDIO_CHANNELS g_handle;

void ExitProcess(int signo)
{
	printf("ExitProcess...\n");

	gpio_destroy(&g_handle, g_handle.audio_ch[g_handle.channel_idx].mclk_gpio);
	gpio_destroy(&g_handle, g_handle.audio_ch[g_handle.channel_idx].lrck_gpio);
	gpio_destroy(&g_handle, g_handle.audio_ch[g_handle.channel_idx].sclk_gpio);
	gpio_destroy(&g_handle, g_handle.audio_ch[g_handle.channel_idx].stdin_gpio);

	pthread_cancel(g_handle.mclkTread);
	pthread_cancel(g_handle.lrckThread);
	pthread_cancel(g_handle.sclkThread);

	pthread_join(g_handle.mclkTread, (void *)NULL);
	pthread_join(g_handle.lrckThread, (void *)NULL);
	pthread_join(g_handle.sclkThread, (void *)NULL);

	pthread_mutex_destroy(&g_handle.sclk_rmutex);
	pthread_cond_destroy(&g_handle.sclk_cond);

	pthread_mutex_destroy(&g_handle.mclk_rmutex);
	pthread_cond_destroy(&g_handle.mclk_cond);

	_exit(1);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("please input parameter by channel, example: audio_wave 0(channel no).\n");
		printf("channe-0: u88\n");
		printf("channe-1: u62\n");
		printf("channe-2: u48\n");
		printf("channe-3: u42\n");
		printf("channe-4: u36\n");
		return -1;
	}
	g_handle.channel_idx = atoi(argv[1]);

	signal(SIGTERM, ExitProcess);
	signal(SIGINT, ExitProcess);
	signal(SIGKILL, ExitProcess);

	for (int i = 0; i < 5; i++)
	{
		g_handle.audio_ch[i].mclk_gpio = g_mclk_gpio_array[i];
		g_handle.audio_ch[i].lrck_gpio = g_lrck_gpio_array[i];
		g_handle.audio_ch[i].sclk_gpio = g_sclk_gpio_array[i];
		g_handle.audio_ch[i].stdin_gpio = g_sdin_gpio_array[i];
	}

	pthread_mutex_init(&g_handle.sclk_rmutex, NULL);
	pthread_mutex_init(&g_handle.mclk_rmutex, NULL);

	pthread_cond_init(&g_handle.sclk_cond, NULL);
	pthread_cond_init(&g_handle.mclk_cond, NULL);

	printf("init finish.\n");

	if (pthread_create(&g_handle.lrckThread, NULL, (void *)&lrck_cycle_fun, (void *)&g_handle))
	{
		perror("pthread_create error.");
	}

	if (pthread_create(&g_handle.sclkThread, NULL, (void *)&sclk_cycle_fun, (void *)&g_handle))
	{
		perror("pthread_create error.");
	}

	if (pthread_create(&g_handle.mclkTread, NULL, (void *)&mclk_cycle_fun, (void *)&g_handle))
	{
		perror("pthread_create error.");
	}

	pthread_join(g_handle.lrckThread, (void *)NULL);
	pthread_join(g_handle.sclkThread, (void *)NULL);
	pthread_join(g_handle.mclkTread, (void *)NULL);

	// 销毁同步对象
	pthread_mutex_destroy(&g_handle.mclk_rmutex);
	pthread_cond_destroy(&g_handle.mclk_cond);

	pthread_mutex_destroy(&g_handle.sclk_rmutex);
	pthread_cond_destroy(&g_handle.sclk_cond);

	return 0;
}
