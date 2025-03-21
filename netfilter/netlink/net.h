#ifndef __NET_H__
#define __NET_H__

#include <linux/icmp.h>
#include <linux/inet.h> /*in_aton()*/
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>/*NF_IP_PRE_FIRST*/
#include <linux/netlink.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/socket.h>/*PF_INET*/
#include <linux/string.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/version.h>
#include <net/sock.h>

#define NETLINK_TEST 30

/* 调试信息 */
#define LOGMSG(fmt, arg...) printk("[func:%s,line:%d]: " fmt, __FUNCTION__, __LINE__, ##arg);

/* 错误信息 */
#define LOGERR(fmt, arg...) printk("[func:%s,line:%d]: " fmt, __FUNCTION__, __LINE__, ##arg);

/* 断言 */
#define ASSERT(expr)                                                                                                   \
	if (unlikely(!(expr)))                                                                                             \
	{                                                                                                                  \
		printk("Assertion failed! %s,%s,%s,line=%d\n", #expr, __FILE__, __func__, __LINE__);                           \
	}
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

typedef struct _fragmemt_packet
{
	int mflag;
	int offset;
	char body[MAX_MSG_LEN];
} fragmemt_packet;

#endif