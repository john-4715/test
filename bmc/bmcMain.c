/*********************************************************************************
 *      Copyright:  (C) 2023 Hejr
 *                  All rights reserved.
 *
 *       Filename:  usart_test.c
 *    Description:  串口测试
 *
 *        Version:  1.0.0(08/27/2018)
 *         Author:  yanhuan <yanhuanmini@foxmail.com>
 *      ChangeLog:  1, Release initial version on "08/23/2018 17:28:51 PM"
 *
 ********************************************************************************/

#include <errno.h> /*错误号定义*/
#include <fcntl.h> /*文件控制定义*/
#include <pthread.h>
#include <stdio.h>	/*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h> /*PPSIX 终端控制定义*/
#include <unistd.h>	 /*Unix 标准函数定义*/

#include "bmc.h"
#include "sys_info.h"
#include "usart.h"
#include "utils.h"

BMC_HANDLE_t g_handle;
unsigned int crc111 = 0xffffffff;
int main(int argc, char **argv)
{
	int fd = -1; // 文件描述符，先定义一个与程序无关的值，防止fd为任意值导致程序出bug
	int ret;	 // 返回调用函数的状态
	init_crc32_table();
	char tmp[12] = {0x01, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x00, 0x80, 0x06, 0x00, 0x00, 0x00};
	int crcval = crc32_cal(crc111, (unsigned char *)tmp, 12);
	char *p = (char *)&crcval;
	printf("crc=");
	for (int i = 0; i < 4; i++)
	{
		printf("%02x ", p[i]);
	}
	printf("\n");

	fd = UART0_Open("/dev/ttyUL1");
	if (fd < 0)
	{
		printf("UART0_Open failed.\n");
		return -1;
	}

	do
	{
		ret = UART0_Init(fd, 115200, 0, 8, 1, 'N');
		printf("Set Port Exactly!\n");
		sleep(1);
	} while (FALSE == ret || FALSE == fd);

	g_handle.fd = fd;
	g_handle.sendlist = Create_Linklist();

	pthread_mutex_init(&g_handle.sendlist_mutex, NULL);
	pthread_mutex_init(&g_handle.send_cond_lock, NULL);
	pthread_cond_init(&g_handle.send_cond, NULL);

	printf("bmc init success.\n");

	pthread_t cycleThread, postThread, sendThread;
	if (pthread_create(&cycleThread, NULL, (void *)&report_timer_func, (void *)&g_handle))
	{
		perror("pthread_create error.");
	}

	if (pthread_create(&postThread, NULL, (void *)&receive_func, (void *)&g_handle))
	{
		perror("pthread_create error.");
	}

	if (pthread_create(&sendThread, NULL, (void *)&send_uart_func, (void *)&g_handle))
	{
		perror("pthread_create error.");
	}

	pthread_join(cycleThread, (void *)NULL);
	pthread_join(postThread, (void *)NULL);
	pthread_join(sendThread, (void *)NULL);

	UART0_Close(fd);
	Linklist_free(g_handle.sendlist);
	pthread_mutex_destroy(&g_handle.sendlist_mutex);
	pthread_mutex_destroy(&g_handle.send_cond_lock);
	pthread_cond_destroy(&g_handle.send_cond);
	return 0;
}