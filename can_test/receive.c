#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

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
	sprintf(cmd, "ip link set %s type can bitrate %d ", dev, baud);
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

	// 将套接字与can1 绑定
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
	int s, nbytes;
	struct can_frame frame;
	struct can_filter rfilter[1];
	unsigned int canno;
	unsigned int baud;

	if (argc < 3)
	{
		fprintf(stderr, "\nUsage:\t%s { 0|1|2(cannum) } [ baud ]\n", argv[0]);
		exit(1);
	}
	canno = atoi(argv[1]);
	baud = atoi(argv[2]);

#if 1
	char cmd[256] = {0};
	// D2000 的 CAN 总线与其它 IO 管脚复用，需先配置复用寄存器为 CAN0、CAN1、CAN2 总线功能
	// 执行寄存器初始化：./rw -w 4 0x28180204 0x89999990
	sprintf(cmd, "%s", "./rw -w 4 0x28180204 0x89999990");
	printf("cmd:%s\n", cmd);
	system(cmd);
#endif

	s = CanInit(canno, baud);
	// 定义接收规则，只接收表示符等于0x11 的报文
	rfilter[0].can_id = 0x11;
	rfilter[0].can_mask = CAN_SFF_MASK;
	// 设置过滤规则
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

	while (1)
	{
		nbytes = read(s, &frame, sizeof(frame)); // 接收报文
		// 显示报文
		if (nbytes > 0)
		{
			printf("ID=%03X, DLC=%d, data=%02X %02X %02X %02X %02X %02X %02X %02X \n", frame.can_id, frame.can_dlc,
				   frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5],
				   frame.data[6], frame.data[7]);
		}
	}
	close(s);
	return 0;
}