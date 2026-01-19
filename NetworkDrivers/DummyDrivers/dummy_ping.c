#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/rtnetlink.h>
#include <linux/net_tstamp.h>
#include <net/rtnetlink.h>
#include <linux/u64_stats_sync.h>
#include <linux/icmp.h>
#include <linux/ip.h>

#define DRV_NAME	"dummy"
#define DRV_VERSION	"1.0"

extern int32_t lt_request_irq(bool mode, int32_t (*rx_fn)(struct sk_buff *skb));

extern int lt_hw_tx(struct sk_buff *skb);

struct net_device *dev_dummy;

int32_t dummy_eth_rx(struct sk_buff *skb)
{
	struct iphdr *iph = NULL;
	struct icmphdr *icmph = NULL;
	int32_t addr = 0;
	char eth_addr[ETH_ALEN];

	if (skb == NULL) {
		printk(KERN_ERR "DETH: skb is null\n");
		return -EINVAL;
	}
	/* Mangel the packet to send ICMP/ping reply */
	iph = ip_hdr(skb);
	if (iph && iph->protocol == IPPROTO_ICMP) {
		__wsum csum = 0;
		
		icmph = icmp_hdr(skb);
		if (icmph == NULL) {
			printk(KERN_ERR "DETH: no such ICMPH header\n");
			goto free;
		}
		print_hex_dump(KERN_ERR, "DETH B: ", 0, 16, 1, skb->data, skb->len, 0);
		/* Alter MAC addresses */
		memcpy(eth_addr, skb->data, ETH_ALEN);
		memmove(skb->data, skb->data + ETH_ALEN, ETH_ALEN);
		memcpy(skb->data + ETH_ALEN, eth_addr, ETH_ALEN);
		/* Alter IP addresses */
		addr = iph->daddr;
		iph->daddr = iph->saddr;
		iph->saddr = addr;
		/* ICMP echo reply */
		icmph->type = ICMP_ECHOREPLY;
		icmph->checksum = 0;
		csum = csum_partial((u8 *)icmph, ntohs(iph->tot_len) - 
				(iph->ihl * 4), csum);
		icmph->checksum = csum_fold(csum);
		print_hex_dump(KERN_ERR, "DETH A: ", 0, 16, 1, skb->data, skb->len, 0);
		/* Pass frame up. XXX: ned to enable hairpin, as same netdev? */
		printk(KERN_ERR "DETH: ping packet came pushing up\n");
		/* Dev is same here no need to assign - skb->dev = dev */
		skb->protocol = eth_type_trans(skb, skb->dev);
		netif_rx(skb);
	} else {
		printk(KERN_ERR "DETH: not a ping packet\n");
		goto free;
	}

	return 0;

free:
	dev_kfree_skb(skb);
	return 0;
}



