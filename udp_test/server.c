// udpSer.c
// gcc udpSer.c -o udpSer
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int sockfd = -1;
void ExitProcess(int signo)
{
	if (sockfd > 0)
	{
		close(sockfd);
	}
	fprintf(stdout, "release over\n");
	_exit(1);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("please input server ip.\n");
		return -1;
	}
	signal(SIGTERM, ExitProcess);
	signal(SIGINT, ExitProcess);
	signal(SIGKILL, ExitProcess);
	char serverip[64];
	sprintf(serverip, "%s", argv[1]);
	// 1、创建UDP套接字socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
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
	servaddr.sin_addr.s_addr = inet_addr(serverip);
	// servaddr.sin_addr.s_addr = inet_addr(INADDR_ANY);

	// 3、绑定 bind
	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind error");
		return -1;
	}

	// 4、使用 sendto、recvfrom 交互数据
	printf("UdpSer sockfd=%d, start \n", sockfd);
	char buffer[1024];
	while (1)
	{
		struct sockaddr_in clientaddr;
		bzero(&clientaddr, sizeof(clientaddr));
		socklen_t clientlen = sizeof(clientaddr);
		int nlen = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr, &clientlen);
		if (nlen > 0)
		{
			buffer[nlen] = 0;
			printf("recv sockfd=%d %d byte, [%s] clientlen=%d, cliIp=%s, cliPort=%d\n", sockfd, nlen, buffer, clientlen,
				   inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
			printf("data:");
			for (int i = 0; i < nlen; i++)
			{
				printf("0x%02x ", buffer[i]);
			}
			printf("\n");
			// sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&clientaddr, clientlen);
		}
	}

	// 5、关闭
	close(sockfd);

	return 0;
}
