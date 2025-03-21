#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <asm/types.h>
#include <linux/dma-mapping.h>
#include <sys/mman.h>
 
int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    struct iovec iov;
    char *kernel_buffer;
    size_t len = 1024;
 
    // 创建UDP套接字
    sock = socket(AF_INET, SOCK_DGRAM, 0);
 
    // 填写服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(12345);
 
    // 为DMA分配内存
    kernel_buffer = (char *)dma_alloc_coherent(NULL, len, NULL, GFP_KERNEL);
 
    // 填充数据
    memcpy(kernel_buffer, "Hello, UDP!", 13);
 
    // 准备发送
    iov.iov_base = kernel_buffer;
    iov.iov_len = len;
 
    // 通过DMA发送数据
    if (sendmsg(sock, &msg, 0) < 0) {
        perror("sendmsg failed");
        exit(1);
    }
 
    // 释放DMA分配的内存
    dma_free_coherent(NULL, len, kernel_buffer, (dma_addr_t)iov.iov_base);
 
    close(sock);
    return 0;
}
