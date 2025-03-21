/*
 * author: hejunrong
 * date	 : 2024-5-24
 */
#include "net.h"

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
	// struct ethhdr *eth = NULL;
	struct iphdr *iph = NULL;
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *nl_skb = NULL;

	int skb_len = 0;

	ASSERT(skb != NULL);
	ASSERT(sk != NULL);
	if (g_nlpid < 0)
		return 0;

	// eth = eth_hdr(skb);
	iph = ip_hdr(skb);
	skb_len = iph->tot_len;

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
	memcpy(NLMSG_DATA(nlh), (char *)iph, htons(iph->tot_len));
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

static int netlink_send_msg(struct sock *sk, char *buf, int len)
{
	struct sk_buff *nl_skb;
	struct nlmsghdr *nlh;
	int ret;

	/*创建sk_buff空间*/
	nl_skb = nlmsg_new(len, GFP_ATOMIC);
	if (!nl_skb)
	{
		printk("netlink alloc failure.\n");
		return -1;
	}

	/*设置netllink消息头*/
	nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_TEST, len, 0);
	if (nlh == NULL)
	{
		printk("nlmsg_put failure.\n");
		nlmsg_free(nl_skb);
		return -1;
	}

	/*拷贝数据发送*/
	memcpy(nlmsg_data(nlh), buf, len);
	netlink_unicast(sk, nl_skb, g_nlpid, MSG_DONTWAIT);
}

/*
 * netfilter PRE_ROUTING钩子
 * */
unsigned int pre_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in,
							  const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	int ret = 0;

#if 1
	if (skb == NULL)
	{
		return -1;
	}
	if (!g_nl_sk)
	{
		return -1;
	}
	if (g_nlpid == 0)
	{
		return -1;
	}

	if (skb->protocol == htons(ETH_P_IP))
	{
		struct ethhdr *ehdr;
		struct iphdr *iph;
		struct udphdr *udp_hdr;
		struct tcphdr *tcp_hdr;
		struct icmphdr *icmp_hdr;

		struct nlmsghdr *nlh;
		struct sk_buff *nl_skb;

		// DbgPrint("Send packages to user\n");

		nl_skb = alloc_skb(NLMSG_SPACE(MAX_MSG_LEN), GFP_ATOMIC);
		// nl_skb = alloc_skb(NLMSG_SPACE(0), GFP_ATOMIC);
		if (nl_skb == NULL)
		{
			printk("allocate skb failed.\n");
			return -1;
		}

		ehdr = eth_hdr(skb);
		if (NULL == ehdr)
		{
			printk("ehdr == null.\n");
			return -1;
		}
		iph = ip_hdr(skb);
		if (NULL == iph)
		{
			printk("iph == null.\n");
			return -1;
		}

		nlh = nlmsg_put(nl_skb, 0, 0, 0, NLMSG_SPACE(MAX_MSG_LEN) - sizeof(struct nlmsghdr), 0);
		NETLINK_CB(nl_skb).nsid = 0;

		// DbgPrint("Data length: %d, len=%d\n", htons(iph->tot_len) + ETH_HLEN, NLMSG_SPACE(1514));

		if (htons(iph->tot_len) + ETH_HLEN < MAX_MSG_LEN)
		{
			// Copy data to nlh
			memcpy(NLMSG_DATA(nlh), (char *)ehdr, htons(iph->tot_len) + ETH_HLEN);
			netlink_unicast(g_nl_sk, nl_skb, g_nlpid, MSG_DONTWAIT);
		}
		else
		{
			printk("capture packet datalen overflow,iph->tot_len=%d.\n", htons(iph->tot_len));
			printk("iph->protocol=%d\n", iph->protocol);
		}
	}
	else if (skb->protocol == htons(ETH_P_ARP))
	{
		printk("capture arp packet.\n");
	}
	else if (skb->protocol == htons(ETH_P_PPP_SES))
	{
		printk("capture PPPoE discovery messages.\n");
	}
	else if (skb->protocol == htons(ETH_P_IPV6))
	{
		printk("capture ipv6 packet.\n");
	}
	else
	{
		printk("skb->protocol=%d.\n", skb->protocol);
	}

#else
	struct nlmsg nl;

	memset(&nl, 0, sizeof(struct nlmsg));
	nl.type = NLMSG_TYPE_KERNEL;
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
	.hooknum = NF_INET_PRE_ROUTING,
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