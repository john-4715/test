#ifndef __SYS_INFO_H__
#define __SYS_INFO_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#define MAXBUFSIZE 1024
#define WAIT_SECOND 3 // 暂停时间，单位为“秒”
#define CPU_CORE_NUM 8

typedef struct occupy
{
	char name[20];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
} CPU_OCCUPY;

typedef struct PACKED
{
	char name[20];
	long total;
	char name2[20];
	long free;
} MEM_OCCUPY;

typedef struct cpu_temp_array
{
	int temperature[CPU_CORE_NUM];
} CPU_TEMP_ARRAY;

typedef struct cpu_freq_array
{
	int frequency[CPU_CORE_NUM];
} CPU_FREQ_ARRAY;

int get_sys_version(struct utsname *sysinfo);

int get_cpu_status(unsigned short *status);

float cal_occupy(CPU_OCCUPY *, CPU_OCCUPY *);

void get_occupy(CPU_OCCUPY *);

int get_cpu_temperature(CPU_TEMP_ARRAY *temp_arry, int *corenum);

int get_cpu_frequency(CPU_FREQ_ARRAY *freq_arry, int *corenum);

int get_mem_occupy(float *memory_usage);

float get_io_occupy();

void get_disk_occupy(char *reused);

int getNetcardName(char *net_name);

int get_mac_address(char *net_name, char *macAddr);

int get_ip_address(const char *net_name, char *ip_addr);

int get_netmask_address(const char *net_name, char *netmask_addr);

int getCurrentDownloadRates(long int *save_rate);

void report_timer_func(void *args);

void receive_func(void *args);

#endif