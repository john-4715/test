#include "lwip/api.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/udp.h"
#include <pthread.h>

/* 创建UDP服务器 */
void start_udp_server(void)
{
	struct udp_pcb *upcb;
	ip_addr_t multicast_ip;
	upcb = udp_new();

	/* 设置多播IP地址 */
	IP4_ADDR(&multicast_ip, 192, 168, 18, 250);

	/* 绑定到本地端口 */
	udp_bind(upcb, IP_ANY_TYPE, 1234);

	/* 加入多播组 */
	udp_join_multicast_group(upcb, &multicast_ip);

	/* 设置接收回调函数 */
	udp_recv(upcb, udp_receive, NULL);
}

/* UDP数据接收回调函数 */
void udp_receive(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	if (p != NULL)
	{
		/* 处理接收到的数据 */
		printf("Received UDP packet: %s\n", (char *)p->payload);

		/* 释放pbuf */
		pbuf_free(p);
	}
}

/* 主函数 */
int main(void)
{
	/* 初始化lwIP */
	lwip_init();

	/* 启动UDP服务器 */
	start_udp_server();

	/* 主循环 */
	while (1)
	{
		/* lwIP主动周期性调用 */
		sys_check_timeouts();
	}

	return 0;
}