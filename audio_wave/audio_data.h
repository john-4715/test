#ifndef __AUDIO_DATA_H__
#define __AUDIO_DATA_H__

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct _AUDIO_DATA
{
	int mclk_gpio;
	int lrck_gpio;
	int sclk_gpio;
	int stdin_gpio;
} AUDIO_DATA;

typedef struct _AUDIO_CHANNELS
{
	pthread_t lrckThread;
	pthread_t sclkThread;
	pthread_t mclkTread;

	pthread_mutex_t sclk_rmutex;
	pthread_cond_t sclk_cond;

	pthread_mutex_t mclk_rmutex;
	pthread_cond_t mclk_cond;

	int channel_idx;
	AUDIO_DATA audio_ch[5];
} AUDIO_CHANNELS;

#endif