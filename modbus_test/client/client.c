
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 502
#define BUFFER_SIZE 256

int connect_to_server()
{
	int sock = 0;
	struct sockaddr_in serv_addr;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address / Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	return sock;
}

int main()
{
	int sock = connect_to_server();
	if (sock < 0)
		return -1;

	unsigned char modbus_request[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};

	send(sock, modbus_request, sizeof(modbus_request), 0);
	printf("Modbus request sent\n");

	char buffer[BUFFER_SIZE] = {0};
	ssize_t valread = read(sock, buffer, BUFFER_SIZE);
	if (valread > 0)
	{
		printf("Response received: ");
		for (int i = 0; i < valread; ++i)
			printf("%02x ", (unsigned char)buffer[i]);
		printf("\n");
	}

	close(sock);
	return 0;
}
