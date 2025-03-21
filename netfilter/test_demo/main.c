/*
 * author: hejunrong
 * date	 : 2024-5-24
 */
#include "utils.h"
#include <asm/types.h>
#include <errno.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <linux/udp.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_PAYLOAD 1580

// netlink消息
typedef struct _user_msg_info
{
	struct nlmsghdr nlmsg;
	char msg[MAX_PAYLOAD];
} user_msg_info;

#define FREE_INIT(ptr)                                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		free(ptr);                                                                                                     \
		ptr = NULL;                                                                                                    \
	} while (0)

/* 消息最大值 */
#define MAX_MSG_LEN 1024
enum
{
	NLMSG_TYPE_NONE = 0,
	NLMSG_TYPE_SETPID, /* 设置PID */
	NLMSG_TYPE_KERNEL, /* 消息来自内核 */
	NLMSG_TYPE_APP,	   /* 消息来自应用层 */
};
struct nlmsg
{
	int type;			   /* 消息类型 */
	int len;			   /* 消息长度，包括头部 */
	char msg[MAX_MSG_LEN]; /* 消息正文 */
};

#define NETLINK_TEST 30

/*
 * 打开netlink
 * return: 0, 成功; -1, 失败
 * */
int netlink_open(void)
{
	struct sockaddr_nl saddr;
	int sockfd = -1, ret = 0;

	sockfd = socket(PF_NETLINK, SOCK_RAW, NETLINK_TEST);
	if (sockfd < -1)
	{
		perror("create socket!\n");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.nl_family = PF_NETLINK;
	saddr.nl_pid = getpid(); // self pid
	saddr.nl_groups = 0;	 // multi cast

	ret = bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret < 0)
	{
		perror("bind failed!\n");
		close(sockfd);
		return -1;
	}

	return sockfd;
}
/*
 * 发送信息
 * param[in]: sockfd
 * param[in]: pmsg, 待发送信息
 *
 * return: 0, 发送成功; -1: 发送失败
 * */
int netlink_send(int sockfd, struct nlmsg *pmsg)
{
	struct msghdr msg;
	struct iovec iov;
	struct nlmsghdr *nlh = NULL;

	int msglen = pmsg->len;
	int totlen = NLMSG_SPACE(pmsg->len);
	int ret = 0;

	nlh = malloc(totlen);
	if (!nlh)
	{
		fprintf(stderr, "malloc failed!\n");
		return -1;
	}
	nlh->nlmsg_len = totlen;
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_pid = getpid();

	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	memcpy(NLMSG_DATA(nlh), pmsg, msglen);
	ret = sendmsg(sockfd, &msg, 0);
	if (ret < 0)
	{
		fprintf(stderr, "sendmsg failed!\n");
		FREE_INIT(nlh);
		return -1;
	}
	return 0;
}
/*
 * 接收信息
 * param[in]: sockfd
 * param[out]: pmsg
 *
 * return 0, 成功; -1, 失败
 * */
int netlink_recv(int sockfd, struct nlmsg *pmsg)
{
	struct msghdr msg;
	struct iovec iov;
	struct nlmsghdr *nlh = NULL;

	int msglen = sizeof(*pmsg);
	int totlen = NLMSG_SPACE(sizeof(*pmsg));
	int ret = 0;

	nlh = malloc(totlen);
	if (!nlh)
	{
		fprintf(stderr, "malloc failed!\n");
		return -1;
	}

	iov.iov_base = (void *)nlh;
	iov.iov_len = totlen;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	memcpy(NLMSG_DATA(nlh), pmsg, msglen);
	ret = recvmsg(sockfd, &msg, 0);
	if (ret < 0)
	{
		fprintf(stderr, "recvmsg failed!\n");
		FREE_INIT(nlh);
		return -1;
	}
	memcpy(pmsg, NLMSG_DATA(nlh), msglen);
	return 0;
}

int netlink_recv1(int sockfd, char *buff, int *len)
{
	struct msghdr msg;
	struct iovec iov;
	struct nlmsghdr *nlh = NULL;

	int msglen = MAX_MSG_LEN;
	int totlen = MAX_MSG_LEN + 2 * sizeof(int);
	int ret = 0;

	nlh = malloc(totlen);
	if (!nlh)
	{
		fprintf(stderr, "malloc failed!\n");
		return -1;
	}

	iov.iov_base = (void *)nlh;
	iov.iov_len = totlen;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	// memcpy(NLMSG_DATA(nlh), buff, msglen);
	ret = recvmsg(sockfd, &msg, 0);
	if (ret < 0)
	{
		fprintf(stderr, "recvmsg failed!\n");
		FREE_INIT(nlh);
		return -1;
	}

	memcpy(buff, NLMSG_DATA(nlh), msglen);

	*len = msglen;
	return 0;
}
/*
 * 关闭netlink
 * param[in]: sockfd, netlink socket号
 * */
void netlink_close(int sockfd)
{
	if (sockfd > 0)
		close(sockfd);
}

int parse_packet(const char *data, int len)
{
	struct iphdr *ip_hdr;
	struct udphdr *udp_hdr;
	struct tcphdr *tcp_hdr;
	struct icmphdr *icmp_hdr;
	struct ether_header *ether;

	ip_hdr = (struct iphdr *)(data + sizeof(struct ether_header));
	char srcIp[32] = {0}, dstIp[32] = {0};
	char srcNet[32] = {0}, dstNet[32] = {0};
	convertIp(srcIp, ip_hdr->saddr);
	convertIp(dstIp, ip_hdr->daddr);
	printf("srcIp:%s,dstIp:%s,\n", srcIp, dstIp);
	if (ip_hdr->protocol == 0x11)
	{
		udp_hdr = (struct udphdr *)(data + sizeof(struct ether_header) + sizeof(struct iphdr));

		printf("srcport=%d,dstport=%d\n", ntohs(udp_hdr->source), ntohs(udp_hdr->dest));
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd = -1;
	int ret = 0;
	char buffer[MAX_MSG_LEN];
	struct nlmsg msg;

	char *psend = "msg from app!";

	memset(&msg, 0, sizeof(msg));
	/* 创建netlink socket */
	sockfd = netlink_open();
	if (sockfd < 0)
	{
		fprintf(stderr, "netlink_open failed!\n");
		return -1;
	}
	/* 将进程号通知内核 */
	msg.type = NLMSG_TYPE_SETPID;
	msg.len = strlen(psend) + offsetof(struct nlmsg, msg) + 1;
	memcpy(msg.msg, psend, strlen(psend));
	ret = netlink_send(sockfd, &msg);
	if (ret < 0)
	{
		fprintf(stderr, "netlink_send failed!\n");
		return -1;
	}

	/* 接收消息 */
	while (1)
	{
#if 0
		ret = netlink_recv(sockfd, &msg);
		if (ret < 0)
		{
			fprintf(stderr, "netlink_recv failed!\n");
			return -1;
		}
		printf("msg: %s\n", msg.msg);
#else
		memset(buffer, 0, MAX_MSG_LEN);
		int len = 0;
		ret = netlink_recv1(sockfd, buffer, &len);
		if (ret < 0)
		{
			fprintf(stderr, "netlink_recv failed!\n");
			return -1;
		}
		if (len > 0)
		{
			parse_packet(buffer, len);
		}
		else
		{
			break;
		}

#endif
	}

	/* 关闭netlink */
	netlink_close(sockfd);
	return 0;
}