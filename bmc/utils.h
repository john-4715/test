#ifndef __UTILS_H__
#define __UTILS_H__

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_DATA_SIZE 1024

typedef struct _CUR_TIME_PARA
{
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
} CUR_TIME_PARA;

void init_crc32_table(void);

unsigned int crc32_cal(unsigned int crc, unsigned char *buffer, unsigned int size);

int hstrtoi(char *s);

uint64_t hstrtol(char *s);

void getLocalTime(CUR_TIME_PARA *curtime);

double GetCurrentTime(void);

double GetMilliTime();

int file_exist(const char *path);
#endif