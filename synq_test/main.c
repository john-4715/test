#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 3333

#define AXI4_INTERFACE 0x83c80000
#define DATA_LEN 1024

unsigned int *map_base0;

void udp_server(void)
{
	int sockfd;
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("create socket false\n");
		exit(1);
	}
	socklen_t len;

	struct sockaddr_in server_addr;
	int n = 0x01;
	int opt = 1;

	len = sizeof(server_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("can not bind\n");
		exit(1);
	}

	// while (1)
	{
		char buf[DATA_LEN];
		*map_base0 = 0x05;
		memcpy(buf, map_base0, 4);

		sendto(sockfd, map_base0, n, 0, (struct sockaddr *)&server_addr, len);
		printf("\n%x  send data to client: %s\n", map_base0, buf);
		printf("\n========wait for client's request========\n");
		n = recvfrom(sockfd, map_base0, DATA_LEN, 0, (struct sockaddr *)&server_addr, &len);
		printf("\n%x  receive client's data: %s\n", map_base0, buf);
	}
	close(sockfd);
}

void pl_peripheral(void)
{

	int devfd;

	if ((devfd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
	{
		printf("can not open /dev/mem \n");
		exit(1);
	}
	printf("\n open /dev/mem successful\n");

	map_base0 = mmap(NULL, DATA_LEN * 4, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, AXI4_INTERFACE);
	if (map_base0 == 0)
	{
		printf("NULL pointer \n");
	}
	else
	{
		printf("mmap successful\n");
	}
}

#define BRAM_CTRL_0 0x83C08000
#define BRAM_CTRL_1 0x83C08000
#define DATA_LEN 6

int main(int argc, char **argv)
{
	// pl_peripheral();

	// udp_server();

	unsigned int *map_base0;
	unsigned int *map_base1;

	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0)
	{
		printf("can not open /dev/mem \n");
		return (-1);
	}
	printf("/dev/mem is open \n");

	map_base0 = mmap(NULL, DATA_LEN * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BRAM_CTRL_0);
	map_base1 = mmap(NULL, DATA_LEN * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BRAM_CTRL_1);
	if (map_base0 == 0 || map_base1 == 0)
	{
		printf("NULL pointer\n");
	}
	else
	{
		printf("mmap successful\n");
	}

	unsigned long addr;
	unsigned int content;
	int i = 0;

	// printf("\nwrite data to bram\n");
	// for (i = 0; i < DATA_LEN; i++)
	// {
	// 	int offset = 4 * i;
	// 	addr = (unsigned long)(map_base0 + offset);
	// 	content = i + 2;
	// 	map_base0[offset] = content;
	// 	printf("%2dth data, address: 0x%lx data_write: 0x%x\t\t\n", i, addr, content);
	// }

	printf("\nread data from bram\n");
	for (i = 0; i < DATA_LEN; i++)
	{
		int offset = 4 * i;
		printf("11111111111111111\n");
		addr = (unsigned long)(map_base1 + offset);
		printf("22222222222222222\n");
		content = map_base1[offset];
		printf("%2dth data, address: 0x%lx data_read: 0x%x\t\t\n", i, addr, content);
	}

	close(fd);
	munmap(map_base0, DATA_LEN);
	munmap(map_base1, DATA_LEN);

	return 0;
}
