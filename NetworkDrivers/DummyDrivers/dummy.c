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

#define DRV_NAME	"dummy"
#define DRV_VERSION	"1.0"

struct net_device *dev_dummy;

/* fake multicast ability */
static void set_multicast_list(struct net_device *dev)
{
}
#if 0
struct pcpu_dstats {
	u64			tx_packets;
	u64			tx_bytes;
	struct u64_stats_sync	syncp;
};
#endif
static void dummy_get_stats64(struct net_device *dev,
			      struct rtnl_link_stats64 *stats)
{
	int i;
	printk("Dummy: In %s\n", __func__);

	for_each_possible_cpu(i) {
		const struct pcpu_dstats *dstats;
		u64 tbytes, tpackets;
		unsigned int start;

		dstats = per_cpu_ptr(dev->dstats, i);
		do {
			start = u64_stats_fetch_begin(&dstats->syncp);
			tbytes = u64_stats_read(&dstats->tx_bytes);
			tpackets = u64_stats_read(&dstats->tx_packets);
		} while (u64_stats_fetch_retry(&dstats->syncp, start));
		stats->tx_bytes += tbytes;
		stats->tx_packets += tpackets;
	}
}

static netdev_tx_t dummy_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct pcpu_dstats *dstats = this_cpu_ptr(dev->dstats);

	u64_stats_update_begin(&dstats->syncp);
	//TODO 8: Update the packet count (tx_packets, tx_bytes in dstats
	// Use u64_stats_inc and u64_stats_add
	// Add skb->len bytes to tx_bytes
	u64_stats_update_end(&dstats->syncp);

	skb_tx_timestamp(skb);
	//TODO 2.2 Trasmit to the hw by using lt_hw_tx
	return NETDEV_TX_OK;
}

static int dummy_dev_init(struct net_device *dev)
{
	printk("Dummy: In %s\n", __func__);
	dev->dstats = netdev_alloc_pcpu_stats(struct pcpu_dstats);
	if (!dev->dstats)
		return -ENOMEM;

	return 0;
}

static void dummy_dev_uninit(struct net_device *dev)
{
	printk("Dummy: In %s\n", __func__);
	free_percpu(dev->dstats);
}

static int dummy_change_carrier(struct net_device *dev, bool new_carrier)
{
	//TODO 7: Handle carrier change event using netif_carrier_on and netif_carrier_off
	return 0;
}

//TODO 5: Populate the netdev operations
//.ndo_init, .ndo_uninit, .ndo_start_xmit, .ndo_validate_addr
//.ndo_set_rx_mode, .ndo_set_mac_address
//.ndo_get_stats64, .ndo_change_carrier
// use default eth_mac_address and eth_validate_address for not defined ones
static const struct net_device_ops dummy_netdev_ops = {
};

static void dummy_setup(struct net_device *dev)
{
	printk("Dummy: In %s\n", __func__);
	ether_setup(dev);

	/* Initialize the device structure. */
	//TODO 6: Initialize the netdev_ops in net_device structure 
	dev->needs_free_netdev = true;

	/* Fill in device structure with ethernet-generic values. */
	dev->flags |= IFF_NOARP;
	dev->flags &= ~IFF_MULTICAST;
	dev->priv_flags |= IFF_NO_QUEUE;
	dev->features	|= NETIF_F_SG | NETIF_F_FRAGLIST;
	dev->features	|= NETIF_F_ALL_TSO;
	dev->features	|= NETIF_F_HW_CSUM | NETIF_F_HIGHDMA;
	dev->features	|= NETIF_F_GSO_ENCAP_ALL;
	dev->hw_features |= dev->features;
	dev->hw_enc_features |= dev->features;
	eth_hw_addr_random(dev);

	dev->min_mtu = 0;
	dev->max_mtu = 0;
}

static int __init dummy_init_module(void)
{
	int err;
	printk("Dummy: In %s\n", __func__);

	//TODO 1: Allocate the netdev structure and assign to dev_dummy
	if (!dev_dummy)
		return -ENOMEM;
	//TODO 2: Register the netdevice
	if (err < 0)
		goto err;
	/*
	 * TODO 2.1: Invoke lt_request_irq from dummy_hw.c 
	 * to register the irq handler dummy_eth_rx
	 * Copy the dummy_eth_rx from dummy_ping.c
	 */ 

	return 0;

err:
	//TODO 3: De-allocate the netdevice using free_netdev
	return err;
}

static void __exit dummy_cleanup_module(void)
{
	//TODO 4: Unregister the netdevice
}

module_init(dummy_init_module);
module_exit(dummy_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK(DRV_NAME);
MODULE_VERSION(DRV_VERSION);
