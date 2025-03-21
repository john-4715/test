#ifndef __MCLK_H__
#define __MCLK_H__

#include "audio_data.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

int gpio_init(AUDIO_CHANNELS *pHandle, int pin_num);
int write_gpio_value(AUDIO_CHANNELS *pHandle, int pin_num, int value);
int gpio_destroy(AUDIO_CHANNELS *pHandle, int pin_num);

void mclk_cycle_fun(void *arg);
void lrck_cycle_fun(void *arg);
void sclk_cycle_fun(void *arg);
#endif