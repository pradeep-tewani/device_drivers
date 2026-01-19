
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */

#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>
#include <linux/version.h> 	/* LINUX_VERSION_CODE  */

#include "snull.h"

#include <linux/in6.h>
#include <asm/checksum.h>

//TODO 2.1: Declare the static int variable use_napi, assign the value 0 and allow it to set during the load time by making it module param
static int use_napi = 1;
module_param(use_napi, int, 0);

/*
 * A structure representing an in-flight packet.
 */
struct snull_packet {
	struct snull_packet *next;
	struct net_device *dev;
	int	datalen;
	u8 data[ETH_DATA_LEN];
};

int pool_size = 8;
module_param(pool_size, int, 0);

/*
 * This structure is private to each device. It is used to pass
 * packets in and out, so there is place for a packet
 */

struct snull_priv {
	struct net_device_stats stats;
	int status;
	struct snull_packet *ppool;
	struct snull_packet *rx_queue;  /* List of incoming packets */
	int rx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
	struct net_device *dev;
	struct napi_struct napi;
};

static void (*snull_interrupt)(int, void *, struct pt_regs *);

/*
 * Set up a device's packet pool.
 */
void snull_setup_pool(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	int i;
	struct snull_packet *pkt;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct snull_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}

void snull_teardown_pool(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
    
	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		kfree (pkt);
		/* FIXME - in-flight packets ? */
	}
}    

/*
 * Buffer/pool management.
 */
struct snull_packet *snull_get_tx_buffer(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	unsigned long flags;
	struct snull_packet *pkt;
    
	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->ppool;
	if(!pkt) {
		PDEBUG("Out of Pool\n");
		return pkt;
	}
	priv->ppool = pkt->next;
	if (priv->ppool == NULL) {
		printk (KERN_INFO "Pool empty\n");
		//TODO 21: Stop the queue using void netif_stop_queue(struct net_device *dev)
	}
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

void snull_release_buffer(struct snull_packet *pkt)
{
	unsigned long flags;
	struct snull_priv *priv = netdev_priv(pkt->dev);
	
	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
	//TODO 22: Wake up the above layer If queue is stopped & pkt->next is NULL. Use netif_queue_stopped to check if the queue is stopped and netif_wake_queue to wake up the above layer
}

void snull_enqueue_buf(struct net_device *dev, struct snull_packet *pkt)
{
	unsigned long flags;
	struct snull_priv *priv = netdev_priv(dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;  /* FIXME - misorders packets */
	priv->rx_queue = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
}

struct snull_packet *snull_dequeue_buf(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->rx_queue;
	if (pkt != NULL)
		priv->rx_queue = pkt->next;
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

/*
 * Enable and disable receive interrupts.
 */
static void snull_rx_ints(struct net_device *dev, int enable)
{
	//TODO 7: Retrieve the private data field & set the rx_int_enabled field to enable
}
 
/*
 * Open and close
 */

int snull_open(struct net_device *dev)
{
	/* request_region(), request_irq(), ....  (like fops->open) */

	/* 
	 * Assign the hardware address of the board: use "\0SNULx", where
	 * x is 0 or 1. The first byte is '\0' to avoid being a multicast
	 * address (the first byte of multicast addrs is odd).
	 */
	/*
	 * TODO 8: Assign mac address - \0SNUL0 for netdev 0 and \0SNUL1 for netdev1
	 * Use void eth_hw_addr_set(struct net_device *dev, const u8 *addr)
	 */
	//TODO 2.4: if use_napi is set, enable the NAPI by using napi_enable
	//TODO 9: Start the transmit queue using void netif_start_queue(struct net_device *dev)
	return 0;
}

int snull_release(struct net_device *dev)
{
    /* release ports, irq and such -- like fops->close */
	//TODO 10: Stop the queue using void netif_stop_queue(struct net_device *dev)
	//TODO 2.5: if use_napi is set, then disable the NAPI with napi_disable
	return 0;
}

/*
 * Receive a packet: retrieve, encapsulate and pass over to upper levels
 */
void snull_rx(struct net_device *dev, struct snull_packet *pkt)
{
	struct sk_buff *skb;
	struct snull_priv *priv = netdev_priv(dev);

	/*
	 * The packet has been retrieved from the transmission
	 * medium. Build an skb around it, so upper layers can handle it
	 */
	//TODO 18: Allocate the skb of length datalen + 2
	// Use struct sk_buff *dev_alloc_skb(unsigned int length) 
	if (!skb) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "snull rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		goto out;
	}
	//TODO 19: Align to 16B boundary and copy the data to skb
	// Use void skb_reserve(struct sk_buff *skb, int len) to leave a headroom of 2 Bytes. This will align the IP header to 16 bytes - 2 bytes + 14 bytes Ethernet Header
	//For copying the data, use void *skb_put(struct sk_buff *skb, unsigned int len). This returns pointer to current tail of the packet

	/* Write metadata, and then pass to the receive level */

	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += pkt->datalen;
	//TODO 20: Push to the above layer using int netif_rx(struct sk_buff *skb)
  out:
	return;
}
        
/*
 * The typical interrupt entry point
 */
static void snull_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	struct snull_priv *priv;
	struct snull_packet *pkt = NULL;
	/*
	 * As usual, check the "device" pointer to be sure it is
	 * really interrupting.
	 * Then assign "struct device *dev"
	 */
	struct net_device *dev = (struct net_device *)dev_id;
	/* ... and check with hw if it's really ours */

	/* paranoid */
	if (!dev)
		return;

	/* Lock the device */
	priv = netdev_priv(dev);
	spin_lock(&priv->lock);

	/* retrieve statusword: real netdevices use I/O instructions */
	statusword = priv->status;
	priv->status = 0;
	if (statusword & SNULL_RX_INTR) {
		/* send it to snull_rx for handling */
		pkt = priv->rx_queue;
		if (pkt) {
			//TODO 15: Point rx_queue to pkt->next and invoke rx handling
		}
	}
	if (statusword & SNULL_TX_INTR) {
		/* a transmission is over: free the skb */
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += priv->tx_packetlen;
		//TODO 16: Free up the skb, if its not NULL and set skb field to NULL after freeing
	}

	/* Unlock the device and we are done */
	spin_unlock(&priv->lock);
	//TODO 17: Release the packet to the pool, if its not NULL
	return;
}

/*
 * Transmit a packet (low level interface)
 */
static void snull_hw_tx(char *buf, int len, struct net_device *dev)
{
	/*
	 * This function deals with hw details. This interface loops
	 * back the packet to the other snull interface (if any).
	 * In other words, this function implements the snull behaviour,
	 * while all other procedures are rather device-independent
	 */
	struct iphdr *ih;
	struct net_device *dest;
	struct snull_priv *priv;
	u32 *saddr, *daddr;
	struct snull_packet *tx_buffer;

	/*
	 * Ethhdr is 14 bytes, but the kernel arranges for iphdr
	 * to be aligned (i.e., ethhdr is unaligned)
	 */
	ih = (struct iphdr *)(buf+sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
	((u8 *)daddr)[2] ^= 1;

	ih->check = 0;         /* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);

	/*
	 * Ok, now the packet is ready for transmission: first simulate a
	 * receive interrupt on the twin device, then  a
	 * transmission-done on the transmitting device
	 */
	dest = snull_devs[dev == snull_devs[0] ? 1 : 0];
	priv = netdev_priv(dest);
	tx_buffer = snull_get_tx_buffer(dev);

	if(!tx_buffer) {
		PDEBUG("Out of tx buffer, len is %i\n",len);
		return;
	}

	tx_buffer->datalen = len;
	memcpy(tx_buffer->data, buf, len);
	//TODO 13: Enqueue snull packet to the rx_queue of dest
	//TODO 14: Check if RX Interrupt is enabled & then set the SNULL_RX_INTR status and Simulate the RX Interrupt

	priv = netdev_priv(dev);
	priv->tx_packetlen = len;
	priv->tx_packetdata = buf;
	priv->status |= SNULL_TX_INTR;
	//TODO 15: Simulate the TX Interrupt
}

/*
 * Transmit a packet (called by the kernel)
 */
int snull_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data;
	struct snull_priv *priv;

	data = skb->data;
	len = skb->len;
	netif_trans_update(dev);

	/* TODO 11: Extract the snull_priv structure from net_device using netdev_priv
	 * Remember the skb (assign current skb to skb field of priv), 
	 * So we can free it at interrupt time 
     */

	/* actual deliver of data is device-specific, and not shown here */
	//TODO 12: Invoke snull_hw_tx

	return 0; /* Our simple device can not fail */
}

/*
 * Return statistics to the caller
 */
struct net_device_stats *snull_stats(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	return &priv->stats;
}

int snull_header(struct sk_buff *skb, struct net_device *dev,
                unsigned short type, const void *daddr, const void *saddr,
                unsigned len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
	return (dev->hard_header_len);
}

static const struct header_ops snull_header_ops = {
	.create  = snull_header,
	.cache = NULL,
};

//TODO 4: Populate the operations for ndo_open, ndo_stop
// ndo_start_xmit and ndo_get_stats
static const struct net_device_ops snull_netdev_ops = {
};

/*
 * The init function (sometimes called probe).
 * It is invoked by register_netdev()
 */
void snull_init(struct net_device *dev)
{
	struct snull_priv *priv;

    	/* 
	 * Then, assign other fields in dev, using ether_setup() and some
	 * hand assignments
	 */
	ether_setup(dev); /* assign some of the fields */
	//TODO 3: Initialize netdev_ops and header_ops fields of net_device
	/* keep the default flags, just add NOARP */
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_HW_CSUM;
	/*
	 * TODO 5: Then, zero out the priv field. This encloses the statistics
	 * and a few private fields. Retrieve priv field using netdev_priv
	 */

	//TODO 2.3: if NAPI is enabled, then register the snull_poll as NAPI handler with the weight of 2
	spin_lock_init(&priv->lock);
	priv->dev = dev;
	//TODO 6: Enable the receive interrupts using snull_rx_ints & set up the pool
}

/*
 * The devices
 */

struct net_device *snull_devs[2];

/*
 * Finally, the module stuff
 */

void snull_cleanup(void)
{
	int i;
    
	for (i = 0; i < 2;  i++) {
		if (snull_devs[i]) {
			//TODO 23: De-register the Netdev & deallocate the pool
			//TODO 24: Free up the Netdev
		}
	}
	return;
}

int snull_init_module(void)
{
	int result, i, ret = -ENOMEM;

	/*
	 * TODO 2.2: If napi is enabled, use snull_napi_interrupt instead
	 * Copy the same from snull_napi.c
	 */
	snull_interrupt = snull_regular_interrupt;
	/* Allocate the devices */
	//TODO 1: Allocate the snull interfaces sn0 and sn1
	// Use alloc_netdev (refer apis.txt for prototype) 
	// Use snull_init as setup function
	if (snull_devs[0] == NULL || snull_devs[1] == NULL)
		goto out;

	ret = -ENODEV;
	for (i = 0; i < 2;  i++)
		//TODO 2: Register the snull interfaces sn0 and sn1
		// using register_netdev (Refer apis.txt for prototype)
		if (result)
			printk("snull: error %i registering device \"%s\"\n",
					result, snull_devs[i]->name);
		else
			ret = 0;
   out:
	if (ret) 
		snull_cleanup();
	return ret;
}

module_init(snull_init_module);
module_exit(snull_cleanup);

MODULE_AUTHOR("Pradeep Tewani");
MODULE_LICENSE("Dual BSD/GPL");
