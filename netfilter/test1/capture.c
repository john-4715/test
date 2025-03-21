#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/time.h>
#include <net/icmp.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/tcp.h>
#define CHECKSUM_HW 1

unsigned int my_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in,
					 const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph = ip_hdr(skb);
	struct tcphdr *tcph;
	struct udphdr *udph;
	struct icmphdr *icmph;
	struct net_device *master;
	int i = 0, ret = -1;
	int header = 0;
	int index = 0;
	unsigned char *data = NULL;
	int length = 0;
	if (likely(iph->protocol == IPPROTO_TCP))
	{
		printk("tcp protocol\n");
		tcph = tcp_hdr(skb);
		data = skb->data + iph->ihl * 4 + tcph->doff * 4;
		header = iph->ihl * 4 + tcph->doff * 4;
		length = skb->len - iph->ihl * 4 - tcph->doff * 4;
		if (skb->len - header > 0)
		{
			printk("**************now_start_in_data*****************\n");
			printk("header length is %d", header);
			printk("\r\n");
			printk("len-header is %d", skb->len - header);
			printk("\r\n");
			printk("data length is %d", length);
			printk("\r\n");
			if (skb->data_len != 0)
			{
				if (skb_linearize(skb))
				{
					printk("error line skb\r\n");
					printk("skb->data_len %d\r\n", skb->data_len);
					return NF_DROP;
				}
			}
			for (i = 0; i < length; i++)
			{
				printk("%c", data[i]);
			}
			iph->check = 0;
			iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
			if (skb->ip_summed == CHECKSUM_HW)
			{

				tcph->check =
					csum_tcpudp_magic(iph->saddr, iph->daddr, (ntohs(iph->tot_len) - iph->ihl * 4), IPPROTO_TCP,
									  csum_partial(tcph, (ntohs(iph->tot_len) - iph->ihl * 4), 0));
				skb->csum = offsetof(struct tcphdr, check);
			}
		}
	}
	else if (likely(iph->protocol == IPPROTO_UDP))
	{
		printk("udp protocol\n");
		udph = udp_hdr(skb);
		data = skb->data + iph->ihl * 4 + sizeof(struct udphdr);
		header = iph->ihl * 4 + sizeof(struct udphdr);
		length = ntohs(iph->tot_len) - iph->ihl * 4 - sizeof(struct udphdr);
		if (skb->len - header > 0)
		{
			printk("header length is %d", header);
			printk("\r\n");
			printk("len -header is  %d", skb->len - header);
			printk("\r\n");
			printk("data length is %d", length);
			printk("\r\n");
			for (i = 0; i < length; i++)
			{
				printk(" %02x", data[i]);
				if ((i + 1) % 16 == 0)
					printk("\r\n");
			}
			if (skb->data_len != 0)
			{
				if (skb_linearize(skb))
				{
					printk("error line skb\r\n");
					printk("skb->data_len %d\r\n", skb->data_len);
					return NF_DROP;
				}
			}
			for (i = 0; i < length; i++)
			{
				printk("%c", data[i]);
			}
			iph->check = 0;
			iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
			if (skb->ip_summed == CHECKSUM_HW)
			{
				udph->check =
					csum_tcpudp_magic(iph->saddr, iph->daddr, (ntohs(iph->tot_len) - iph->ihl * 4), IPPROTO_UDP,
									  csum_partial(udph, (ntohs(iph->tot_len) - iph->ihl * 4), 0));
			}
			printk("*********************UDPend********************\n");
		}
	}
	else if (likely(iph->protocol == IPPROTO_ICMP))
	{
		printk("icmp protocol\n");
		icmph = icmp_hdr(skb);
		data = skb->data + iph->ihl * 4 + sizeof(struct icmphdr);
		header = iph->ihl * 4 + sizeof(struct icmphdr);
		length = ntohs(iph->tot_len) - iph->ihl * 4 - sizeof(struct icmphdr);
		if (skb->len - header > 0)
		{
			printk("header length is %d", header);
			printk("\r\n");
			printk("len - header is %d", skb->len - header);
			printk("\r\n");
			printk("data length is  %d", length);
			printk("\r\n");
			if (skb->data_len != 0)
			{
				if (skb_linearize(skb))
				{
					printk("error line skb\r\n");
					printk("skb->data_len %d\r\n", skb->data_len);
					return NF_DROP;
				}
			}
			for (i = 0; i < length; i++)
			{
				printk("%c", data[i]);
			}
			printk("\r\n");

			iph->check = 0;
			iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
			if (skb->ip_summed == CHECKSUM_HW)
			{
				icmph->checksum = ip_compute_csum(icmph, (ntohs(iph->tot_len) - iph->ihl * 4));
			}
			printk("*********************ICMPend********************\n");
		}
	}
	return NF_ACCEPT;
}

static struct nf_hook_ops nfho = {
	.hook = (nf_hookfn *)my_func,
	.pf = PF_INET,
	.hooknum = NF_INET_LOCAL_IN,
	.priority = NF_IP_PRI_FIRST,

};

static int __init capture_init(void)
{
	int ret = nf_register_net_hook(&init_net, &nfho);
	if (ret < 0)
	{
		printk(KERN_ERR "nf_register_hook failed\n");
		return -1;
	}
	return 0;
}
static void __exit capture_exit(void) { nf_unregister_net_hook(&init_net, &nfho); }

module_init(capture_init);
module_exit(capture_exit);
MODULE_AUTHOR("AFCC_");
MODULE_LICENSE("GPL");
