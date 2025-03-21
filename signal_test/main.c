#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_LEN 100

void input_handler(int num)
{
	char data[MAX_LEN];
	int len;

	/*  读取并输出 STDIN_FILENO 上的输入 */
	len = read(STDIN_FILENO, &data, MAX_LEN);
	data[len] = 0;
	printf("input available:%s\n", data);
}

int main()
{
	int oflags;

	/*  启动信号驱动机制 */
	signal(SIGIO, input_handler);			 // 连接信号和信号处理函数
	fcntl(STDIN_FILENO, F_SETOWN, getpid()); // F_SETOWN IO控制命令设置设备文件的拥有者为本进程
	oflags = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, oflags | FASYNC); // F_SETFL IO控制命令设置设备文件以支持FASYNC，即异步通知模式

	/* 最后进入一个死循环，仅为保持进程不终止，如果程序中
	 没有这个死循会立即执行完毕 */
	while (1)
	{
		sleep(1);
	}
}