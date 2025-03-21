#include "lwip/api.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/udp.h"
#include <pthread.h>

#define SERVER_PORT 8088

void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	if (p != NULL)
	{
		printf("Received packet: %s\n", (char *)p->payload);
		// 回应客户端
		udp_sendto(upcb, p, addr, port);
		pbuf_free(p);
	}
}

int start_udp_server()
{
	struct udp_pcb *upcb;
	ip_addr_t multicast_ip;

	upcb = udp_new();
	if (!upcb)
	{
		printf("Could not create UDP PCB. Out of memory?\n");
		return -1;
	}

	IP4_ADDR(&multicast_ip, 192, 168, 18, 128); // 使用LwIP的多播地址

	// 绑定UDP端口
	upcb = udp_bind(upcb, IP_ANY_TYPE, SERVER_PORT);
	if (!upcb)
	{
		printf("Unable to bind to port %d\n", SERVER_PORT);
		return -1;
	}

	// 设置接收回调函数
	udp_recv(upcb, udp_receive_callback, NULL);

	printf("UDP server started on port %d\n", SERVER_PORT);
	return 0;
}

int main()
{
	lwip_init(); // 初始化LwIP

	// 启动UDP服务器
	if (start_udp_server() < 0)
	{
		return -1;
	}

	while (1)
	{
		// 执行LwIP主循环
		sys_check_timeouts();
		// poll_udp();
	}

	return 0;
}

void lwip_example_app_platform_assert(const char *msg, int line, const char *file)
{
	printf("Assertion \"%s\" failed at line %d in %s\n", msg, line, file);
	fflush(NULL);
	abort();
}