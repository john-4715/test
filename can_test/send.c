#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

void CANTestMenu(void)
{
	printf("************************************************** \n");
	printf("*  Select CAN bitrate menu \n");
	printf("*  1  5Kbps \n");
	printf("*  2  10Kbps \n");
	printf("*  3  20Kbps \n");
	printf("*  4  50Kbps \n");
	printf("*  5  100Kbps \n");
	printf("*  6  125Kbps \n");
	printf("*  7  250Kbps \n");
	printf("*  8  500Kbps \n");
	printf("*  9  800Kbps \n");
	printf("*  10 1000Kbps \n");
	printf("*  11 exit \n");
	printf("************************************************** \n");

	printf("*  please input CAN bitrate Int Number,press enter end \n");
	printf("************************************************** \n");
}

int CanInit(unsigned int id, unsigned int baud)
{
	int s;
	int ret;
	char dev[8] = {0};
	char cmd[128] = {0};
	struct sockaddr_can addr = {0};
	struct ifreq ifr = {0};

	sprintf(dev, "can%d", id);
	printf("can dev : %s \n", dev);

	// 关闭can设备
	sprintf(cmd, "ifconfig %s down", dev);
	printf(cmd);
	printf("\n");
	if (system(cmd) < 0)
	{
		printf("can device shut down failed  \n");
		return -1;
	}

	// 设置can设备波特率
	bzero(cmd, sizeof(cmd));
	sprintf(cmd, "/sbin/ip link set %s type can bitrate %d ", dev, baud);
	printf(cmd);
	printf("\n");
	if (system(cmd) < 0)
	{
		printf("set can device baud rate failed  \n");
		return -1;
	}

	// 打开can设备
	bzero(cmd, sizeof(cmd));
	sprintf(cmd, "ifconfig %s up", dev);
	printf(cmd);
	printf("\n");
	if (system(cmd) < 0)
	{
		printf("can device open failed  \n");
		return -1;
	}

	// 创建套接字
	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (s < 0)
	{
		perror("can socket");
		return -1;
	}

	strcpy(ifr.ifr_name, dev);
	// 指定can0 设备
	ret = ioctl(s, SIOCGIFINDEX, &ifr);
	if (ret < 0)
	{
		perror("can ioctl");
		return -1;
	}
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	// 将套接字与can0 绑定
	ret = bind(s, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0)
	{
		perror("can bind");
		return -1;
	}

	return s;
}

int main(int argc, char **argv)
{
	int s, nbytes, n;
	long bitrate;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;

	int canno;

	if (argc < 3)
	{
		fprintf(stderr, "\nUsage:\t%s { 0|1|2(cannum) } [ baud ]\n", argv[0]);
		exit(1);
	}
	canno = atoi(argv[1]);
	bitrate = atoi(argv[2]);

#if 1
	char cmd[256] = {0};
	// D2000 的 CAN 总线与其它 IO 管脚复用，需先配置复用寄存器为 CAN0、CAN1、CAN2 总线功能
	// 执行寄存器初始化：./rw -w 4 0x28180204 0x89999990
	sprintf(cmd, "%s", "./rw -w 4 0x28180204 0x89999990");
	printf("cmd:%s\n", cmd);
	system(cmd);
#endif

	CANTestMenu();

	s = CanInit(canno, bitrate);

	if (s < 0)
	{
		printf("CanInit failed \n");
		sleep(1);
		close(s);
		return -1;
	}
	printf("CanInit success\n");

	// frame.can_id = 0x123ab | CAN_EFF_FLAG;
	frame.can_id = 0x11 | CAN_EFF_FLAG;
	frame.can_dlc = 8;

	frame.data[0] = 0x11;
	frame.data[1] = 0x22;
	frame.data[2] = 0x33;
	frame.data[3] = 0xaa;
	frame.data[4] = 0xbb;
	frame.data[5] = 0xcc;
	frame.data[6] = 0xdd;
	frame.data[7] = 0xee;
	// 禁用过滤规则，本进程不接收报文，只负责发送
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

	while (1)
	{
		nbytes = write(s, &frame, sizeof(frame)); // 发送frame
		if (nbytes != sizeof(frame))
		{
			printf("Send Error frame\n!");
			// break; //发送错误，退出
		}
		else
		{
			printf("Send msg success!\n");
			printf("ID=%03X, DLC=%d, data=%02X %02X %02X %02X %02X %02X %02X %02X \n", frame.can_id, frame.can_dlc,
				   frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5],
				   frame.data[6], frame.data[7]);
		}
		sleep(1);
	}
	close(s);
	return 0;
}