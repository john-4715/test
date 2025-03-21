/*********************************************************************************
 *      Copyright:  (C) 2023 Hejr
 *                  All rights reserved.
 *
 *       Filename:  usart.h
 *    Description:  串口配置
 *
 *        Version:  1.0.0(08/27/2018)
 *         Author:  yanhuan <yanhuanmini@foxmail.com>
 *      ChangeLog:  1, Release initial version on "08/23/2018 17:28:51 PM"
 *
 ********************************************************************************/

#ifndef _USART_H
#define _USART_H

// 串口相关的头文件
#include <errno.h>	/*错误号定义*/
#include <fcntl.h>	/*文件控制定义*/
#include <stdio.h>	/*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h> /*PPSIX 终端控制定义*/
#include <unistd.h>	 /*Unix 标准函数定义*/

// 宏定义
#define FALSE -1
#define TRUE 0

// 打开设备
int UART0_Open(char *port);
// 关闭设备
void UART0_Close(int fd);
// 设置串口属性
int UART0_Set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);
// 初始化串口
int UART0_Init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);
// 串口接收
int UART0_Recv(int fd, char *rcv_buf, unsigned long data_len);
// 串口发送
int UART0_Send(int fd, char *send_buf, int data_len);

void send_uart_func(void *args);

#endif