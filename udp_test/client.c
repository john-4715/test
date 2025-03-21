// udpCli.c
// gcc udpCli.c -o udpCli

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("please input server ip.\n");
		return -1;
	}
	char serverip[64];
	sprintf(serverip, "%s", argv[1]);
	// 1、创建UDP套接字socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("socket error");
		return -1;
	}

	// 2、准备服务端ip和端口
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(10086);
	// servaddr.sin_addr.s_addr = inet_addr(serverip);
	if (inet_pton(AF_INET, serverip, &servaddr.sin_addr) <= 0) // 设置本机IP为服务端IP
	{
		perror("inet_pton error");
		return -1;
	}

	char buffer[1024] = "Hello, I am udp client";

	int nlen = 0;
	while (1)
	{
		nlen = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		printf("nlen=%d\n", nlen);
		// 3、使用 sendto、recvfrom 交互数据
		if (nlen < 0)
		{
			if (errno == ECONNREFUSED || errno == EHOSTUNREACH)
			{
				// 服务器不可达，停止发送
				break;
			}
			else
			{
				// 其他错误处理
				perror("sendto error");
			}
		}
		if (nlen == 0)
		{
			break;
		}

		sleep(1);
	}

	// 4、关闭
	close(sockfd);

	return 0;
}
