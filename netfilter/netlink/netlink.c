/*
 * author: hejunrong
 * date	 : 2024-5-24
 */
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
#define MAX_MSG_LEN 1580

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

/* netlink socket */
static struct sock *g_nl_sk = NULL;
static int g_nlpid = -1; /* 应用层接收程序PID */

/*
 * 发送整个从ip头开始的skb数据到应用层
 *
 * param[in]: sk, skb发送目的socket
 * param[in]: skb, 待发送的skb
 * return -1, 失败; 0, 成功
 * */
int nl_sendskb(struct sock *sk, struct sk_buff *skb)
{
	struct ethhdr *eth = NULL;
	struct iphdr *iph = NULL;
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *nl_skb = NULL;

	int skb_len = 0;

	ASSERT(skb != NULL);
	ASSERT(sk != NULL);
	if (g_nlpid < 0)
		return 0;

	eth = eth_hdr(skb);
	iph = ip_hdr(skb);
	skb_len = iph->tot_len + sizeof(struct ethhdr);
	/* NLMSG_SPACE: sizeof(struct nlmsghdr) + len按4字节对齐 */
	nl_skb = alloc_skb(NLMSG_SPACE(skb_len), GFP_ATOMIC);
	if (!nl_skb)
	{
		LOGERR("nl_skb == NULL, failed!\n");
		return -1;
	}

	nlh = nlmsg_put(nl_skb, 0, 0, 0, NLMSG_SPACE(skb_len) - sizeof(struct nlmsghdr), 0);
	// 填充nlmsghdr和IP头部信息
	NETLINK_CB(nl_skb).nsid = 0; /* 0代表数据来自内核, 发送给自己的用户空间进程 */
	memcpy(NLMSG_DATA(nlh), (char *)eth, htons(iph->tot_len + sizeof(struct ethhdr)));
	// 发送消息到用户空间
	netlink_unicast(sk, nl_skb, g_nlpid, MSG_DONTWAIT);

	return NF_ACCEPT;
}
/*
 * 发送字符串到应用层
 *
 * param[in]: sk, 数据发往的socket
 * param[in]: pmsg, 待发送字符串
 * param[in]: msglen, 待发送字符串长度
 *
 * return: -1, 失败; 0, 成功
 * */
int nl_sendmsg(struct sock *sk, struct nlmsg *pmsg)
{
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *nl_skb = NULL;
	int msglen = pmsg->len;

	ASSERT(pmsg != NULL);
	ASSERT(sk != NULL);

	if (g_nlpid < 0)
		return 0;
	nl_skb = alloc_skb(NLMSG_SPACE(msglen), GFP_ATOMIC);
	if (!nl_skb)
	{
		LOGERR("nl_skb == NULL, msglen = %d, failed!\n", msglen);
		return -1;
	}

	nlh = nlmsg_put(nl_skb, 0, 0, 0, NLMSG_SPACE(msglen) - NLMSG_HDRLEN, 0);
	NETLINK_CB(nl_skb).nsid = 0;
	memcpy(NLMSG_DATA(nlh), pmsg, msglen);

	return netlink_unicast(sk, nl_skb, g_nlpid, MSG_DONTWAIT);
}
/*
 * 从应用层接收数据, netlink_kernel_create注册的回调
 * param[in]: skb, 包含netlink数据的skb
 *
 * skb常用操作函数
 * skb_put : skb->tail += len, skb->len += len
 * skb_pull: skb->data += len, skb->len -= len
 * skb_push: skb->data -= len, skb->len += len
 */
static void nl_recvmsg(struct sk_buff *skb)
{
	LOGMSG("nl_recvmsg......\n");
	struct nlmsg *pmsg = NULL;
	struct nlmsghdr *nlh = NULL;
	uint32_t rlen = 0;

	while (skb->len >= NLMSG_SPACE(0))
	{
		nlh = nlmsg_hdr(skb);
		if (nlh->nlmsg_len < sizeof(*nlh) || skb->len < nlh->nlmsg_len)
			return;
		rlen = NLMSG_ALIGN(nlh->nlmsg_len);
		if (rlen > skb->len)
			rlen = skb->len;
		pmsg = (struct nlmsg *)NLMSG_DATA(nlh);
		switch (pmsg->type)
		{
		case NLMSG_TYPE_SETPID:
			g_nlpid = nlh->nlmsg_pid;
			LOGMSG("pid: %d\n", g_nlpid);
			LOGMSG("msg: %s\n", pmsg->msg);
			break;
		case NLMSG_TYPE_KERNEL:
			break;
		case NLMSG_TYPE_APP:
			break;
		}
		/* 获取下一条netlink消息 */
		skb_pull(skb, rlen);
	}
}

/*
 * netfilter PRE_ROUTING钩子
 * */
unsigned int pre_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in,
							  const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	int ret = 0;
	struct nlmsg nl;

	memset(&nl, 0, sizeof(struct nlmsg));
	nl.type = NLMSG_TYPE_KERNEL;
#if 1
	struct ethhdr *eth = NULL;
	struct iphdr *iph = NULL;
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *nl_skb = NULL;

	int skb_len = 0;

	ASSERT(skb != NULL);

	if (g_nlpid < 0)
		return 0;

	eth = eth_hdr(skb);
	iph = ip_hdr(skb);
	skb_len = iph->tot_len + sizeof(struct ethhdr);
	nl.len = skb_len;
	if (nl.len < MAX_MSG_LEN)
	{
		memcpy(nl.msg, eth, skb_len);
		ret = nl_sendmsg(g_nl_sk, &nl);
	}
	else
	{
		LOGERR("data size overflow, nl.len=%d\n", nl.len);
	}

#else
	char *psend = "msg from kernel.";
	nl.len = strlen(psend) + offsetof(struct nlmsg, msg) + 1;
	memcpy(nl.msg, psend, strlen(psend) + 1);
	ret = nl_sendmsg(g_nl_sk, &nl);
#endif

	return NF_ACCEPT;
}

static struct nf_hook_ops packet_simple_nf_opt = {
	.hook = (nf_hookfn *)pre_routing_hook,
	.pf = PF_INET,
	//.hooknum = NF_INET_PRE_ROUTING,
	.hooknum = NF_INET_LOCAL_OUT,
	.priority = NF_IP_PRI_FIRST,
};

struct netlink_kernel_cfg cfg = {
	.input = nl_recvmsg,
};

static int __init nl_init(void)
{
	int ret = 0;

	g_nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST, &cfg);
	if (!g_nl_sk)
	{
		LOGERR("Fail to create netlink socket.\n");
		return -1;
	}

	ret = nf_register_net_hook(&init_net, &packet_simple_nf_opt);
	if (ret < 0)
	{
		LOGMSG("nf_register_hook failed!\n");

		goto sock_release;
	}
	LOGMSG("nl_init ok!\n");
	return 0;

sock_release:
	if (g_nl_sk)
	{
		sock_release(g_nl_sk->sk_socket);
	}

	return -1;
}

static void __exit nl_exit(void)
{
	synchronize_net();
	if (g_nl_sk)
	{
		sock_release(g_nl_sk->sk_socket);
	}

	nf_unregister_net_hook(&init_net, &packet_simple_nf_opt);
	LOGMSG("nl_exit ok!\n");
}

module_init(nl_init);
module_exit(nl_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hoi0714@163.com");