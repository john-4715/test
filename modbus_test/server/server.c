
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 502
#define BUFFER_SIZE 256

int create_socket()
{
	int server_fd;
	struct sockaddr_in address;
	int opt = 1;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	return server_fd;
}

void handle_client(int new_socket)
{
	char buffer[BUFFER_SIZE] = {0};
	ssize_t valread;

	while ((valread = read(new_socket, buffer, BUFFER_SIZE)) > 0)
	{
		printf("Received Modbus request: ");
		for (int i = 0; i < valread; ++i)
			printf("%02x ", (unsigned char)buffer[i]);
		printf("\n");

		// Echo back the same data as a simple response
		send(new_socket, buffer, valread, 0);
		memset(buffer, 0, BUFFER_SIZE);
	}
}

int main()
{
	int server_fd, new_socket;
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

	server_fd = create_socket();

	printf("Server listening on port %d\n", PORT);

	while ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) >= 0)
	{
		printf("Client connected\n");
		handle_client(new_socket);
		close(new_socket);
	}

	close(server_fd);
	return 0;
}
