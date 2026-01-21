#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/of_net.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include "w5500-spi.h"

int w5500_command(struct sped_net *priv, u8 cmd) {
	unsigned long timeout;
	// TODO 3.8: Write the command into Socket Command Register
	w5500_write(priv, SOCK_CR, BLK_SOCK_REGS, cmd); 
		
	timeout = jiffies + msecs_to_jiffies(100);
	u8 rbuf = 0;

	while (w5500_read(priv, SOCK_CR, BLK_SOCK_REGS, &rbuf)) {
		printk("Timeout\n");
		if (time_after(jiffies, timeout))
			return -EIO;
		cpu_relax();
	}

	return 0;
}

//Write a Single byte
int w5500_write(struct sped_net *priv, u16 offset, u8 block, u8 buf) 
{
	/*
	 * TODO: 2.1: Populate the wbuf for writing a single byte 
	 * to SPI Device
	 */
	u8 wbuf[4] = {
		offset >> 8,
		offset & 0xff,
		(block << 3) | CTL_WRITE,
		buf
	};
	/*
	 * TODO: 2.2: Invoke spi_write_then_read(spi_dev, wbuf, wlen, rbuf, rlen)
	 */
	return spi_write_then_read(priv->spidev, wbuf, sizeof(wbuf), NULL, 0);
}

// Read a Single Byte
int w5500_read(struct sped_net *priv, u16 offset, u8 block, u8 *data) 
{
	/*
	 * TODO: 2.3A: Populate the wbuf to be sent for read operation
	 * to SPI Device
	 */
	u8 wbuf[3] = {
		offset >> 8,
		offset & 0xff,
		(block << 3)
	};
	/*
	 * TODO: 2.4: Invoke spi_write_then_read(spi_dev, wbuf, wlen, rbuf, rlen)
	 */
	return spi_write_then_read(priv->spidev, wbuf, sizeof(wbuf), data, 1);
}

int w5500_read16(struct sped_net *priv, u16 offset, u8 block)
{
	/*
	 * TODO: 2.3B: Populate the wbuf to be sent for read operation
	 * to SPI Device
	 */
	u8 cmd[3] = {
		offset >> 8,
		offset & 0xff,
		(block << 3)
	};
	__be16 data;
	int ret;

	/*
	 * TODO: 2.4B: Invoke spi_write_then_read(spi_dev, wbuf, wlen, rbuf, rlen)
	 */
	ret = spi_write_then_read(priv->spidev, cmd, sizeof(cmd), &data, sizeof(data));

	return ret ? ret : be16_to_cpu(data);
}

int w5500_write16(struct sped_net *priv, u16 offset, u8 block, u16 data)
{
	/*
	 * TODO: 2.5: Populate the wbuf for writing 2 bytes
	 * to SPI Device
	 */
	u8 cmd[5] = {
		offset >> 8,
		offset,
		(block << 3) | CTL_WRITE,
		data >> 8,
		data
	};
	// TODO: 2.6: Invoke spi_write_then_read(spi_dev, wbuf, wlen, rbuf, rlen)
	return spi_write_then_read(priv->spidev, cmd, sizeof(cmd), NULL, 0);
}

int w5500_readbulk(struct sped_net *priv, u16 offset, u8 block, 
				u8 *buf, int len)
{
	struct spi_transfer xfer[] = {
		{
			.tx_buf = priv->cmd_buf,
			.len = sizeof(priv->cmd_buf),
		},
		{
			.rx_buf = buf,
			.len = len,
		},
	};
	int ret;

	mutex_lock(&priv->lock);
	priv->cmd_buf[0] = offset >> 8;
	priv->cmd_buf[1] = offset & 0xff;
	priv->cmd_buf[2] = (block << 3);

	ret = spi_sync_transfer(priv->spidev, xfer, ARRAY_SIZE(xfer));

	mutex_unlock(&priv->lock);

	return ret;
}

int w5500_writebulk(struct sped_net *priv, u16 offset, u8 block, 
				u8 *buf, int len)
{
	struct spi_transfer xfer[] = {
		{
			.tx_buf = priv->cmd_buf,
			.len = sizeof(priv->cmd_buf),
		},
		{
			.tx_buf = buf,
			.len = len,
		},
	};
	int ret;

	mutex_lock(&priv->lock);
	priv->cmd_buf[0] = offset >> 8;
	priv->cmd_buf[1] = offset & 0xff;
	priv->cmd_buf[2] = (block << 3) | CTL_WRITE;

	ret = spi_sync_transfer(priv->spidev, xfer, ARRAY_SIZE(xfer));

	mutex_unlock(&priv->lock);

	return ret;
}

static int w5500_writebuf(struct sped_net *priv, u16 offset, u8 block, 
			const u8 *buf, int len)
{
	u16 addr;
	int ret;
	int remain = 0;
	//const u32 mem_start = priv->s0_tx_buf;
	const u16 mem_size = TX_MEM_SIZE;

	offset %= mem_size;
	addr = offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5500_writebulk(priv, addr, BLK_SOCK_TX_BUF, buf, len);
	if (ret || !remain)
		return ret;

	return w5500_writebulk(priv, 0, BLK_SOCK_TX_BUF, buf + len, remain);
}

static int w5500_readbuf(struct sped_net *priv, u16 offset, u8 block, 
							u8 *buf, int len)
{
	u32 addr;
	int remain = 0;
	int ret;
	const u16 mem_size = RX_MEM_SIZE; //priv->s0_rx_buf_size;

	offset %= mem_size;
	addr = offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5500_readbulk(priv, addr, BLK_SOCK_RX_BUF, buf, len);
	if (ret || !remain)
		return ret;

	return w5500_readbulk(priv, 0, BLK_SOCK_RX_BUF, buf + len, remain);
}

int w5500_reset(struct sped_net *priv)
{
	//TODO 3.3: Set the RST Bit in Mode Register using w5500_write
	w5500_write(priv, COMN_MR, BLK_COMN_REGS, COMN_MR_RST); 	
	mdelay(5);
	//TODO 3.4: Set the Ping Block Bit in Mode Register using w5500_write
	w5500_write(priv, COMN_MR, BLK_COMN_REGS, COMN_MR_PB); 	

	return 0;
}

int w5500_hw_reset(struct sped_net *priv)
{
	//TODO 3.2: Reset the Device with w5500_reset
	w5500_reset(priv);	
	//TODO 3.5: Disable the interrupts by writing 0 into IMR Register
	w5500_write(priv, COMN_IMR, BLK_COMN_REGS, 0);
	/*
	 * TODO 3.6: Write a MAC Address by invoking w5500_writebulk
	 * The Register SHAR (Source Hardware Address), 
	 * The MAC address to be set is in netdev->dev_addr, len is ETH_LEN
	 */
	w5500_writebulk(priv, COMN_SHAR, BLK_COMN_REGS, priv->netdev->dev_addr, ETH_ALEN);
	
	/* 
	 * TODO 3.7: Set the socket 0 RX and TX Buffer size 16KB
 	 */
	w5500_write(priv, SOCK_RX_BUF_SZ, BLK_SOCK_REGS, 0x10);
	w5500_write(priv, SOCK_TX_BUF_SZ, BLK_SOCK_REGS, 0x10);
	for (u8 i = 1; i < 8; i++) {
		w5500_write(priv, SOCK_RX_BUF_SZ, BLK_SOCK_REGS + i * 4, 0x0);
		w5500_write(priv, SOCK_TX_BUF_SZ, BLK_SOCK_REGS + i * 4, 0x0);
	}
	if (w5500_read16(priv, COMN_RTR, BLK_COMN_REGS) != 2000) {
		printk("RTR read failed\n");
		return -ENODEV;
	}
	return 0;
}

static void w5500_hw_start(struct sped_net *priv)
{
	u8 mode = SOCK_MR_MACRAW;

	if (!priv->promisc)
			mode |= SOCK_MR_MF;

	w5500_write(priv, SOCK_MR, BLK_SOCK_REGS, mode);
	//TODO 4.1: Send the 'Socket Open' Command using w5500_command
	w5500_command(priv, SOCK_CMD_OPEN);

	u8 resp = 0;
	//TODO 4.2: Read the Socket Status Registers into resp
	w5500_read(priv, SOCK_SR, BLK_SOCK_REGS, &resp);
	printk("Socket Status = %x\n", mode);
	// TODO 4.3: Enable Interrupts for Socket 0
	w5500_write(priv, COMN_IMR, BLK_COMN_REGS, IR_S0);
}

static void w5500_hw_close(struct sped_net *priv)
{
	//TODO 4.4 Disable Interrupts
	w5500_write(priv, COMN_IMR, BLK_COMN_REGS, 0);
	//TODO 4.5 Send the 'Socket Close Command'
	w5500_command(priv, SOCK_CMD_CLOSE);
}

static struct sk_buff *w5500_rx_skb(struct net_device *ndev)
{
	//TODO 7.7: Get the sped_net structure from netdev_priv
	struct sped_net *priv = netdev_priv(ndev);
	struct sk_buff *skb;
	u16 rx_len;
	u16 offset;
	u8 header[2];
	// TODO 7.8: Get the free space in RX buffer by using w5500_read16
	u16 rx_buf_len = w5500_read16(priv, SOCK_RX_RSR, BLK_SOCK_REGS);

	if (rx_buf_len == 0)
		return NULL;
	
	// TODO 7.9: Get the Read Pointer by using w5500_read16
	offset = w5500_read16(priv, SOCK_RX_RD, BLK_SOCK_REGS);
	// TODO 7.10: Read the 2 bytes header from the RX buffer by using w5500_readbuf
	w5500_readbuf(priv, offset, BLK_SOCK_RX_BUF, header, 2);
	rx_len = get_unaligned_be16(header) - 2;

	/* 
     * TODO 7.11: Allocate a socket buffer
	 * Use netdev_alloc_skb_ip_align(dev, length) which ensures that 
	 * IP header will be correctly aligned in memory
	 */
	skb = netdev_alloc_skb_ip_align(ndev, rx_len);
	if (unlikely(!skb)) {
		w5500_write16(priv, SOCK_RX_RD, BLK_SOCK_REGS, offset + rx_buf_len);
		w5500_command(priv, SOCK_CMD_RECV);
		ndev->stats.rx_dropped++;
		return NULL;
	}

	//TODO 7.12: Extend the data area of skb by rx_len using skb_put
	skb_put(skb, rx_len);
	//TODO 7.13: Read the rx_len data into skb->data by using w5500_readbuff
	w5500_readbuf(priv, offset + 2, BLK_SOCK_RX_BUF, skb->data, rx_len);
	//TODO 7.14: Update the Read Pointer by using w5500_write16
	w5500_write16(priv, SOCK_RX_RD, BLK_SOCK_REGS, offset + 2 + rx_len);
	//TODO 7.15: Finally, receive the data by issuing SOCK_CMD_RECV command
	w5500_command(priv, SOCK_CMD_RECV);
	
	/*
	 * TODO 7.16: Get the Received Packet's protocol ID by using
	 * eth_type_trans(skb, dev) and assign it to skb->protocol
     */
	skb->protocol = eth_type_trans(skb, ndev);

	//TODO 7.17: Update the stats by updating rx_packets and rx_bytes in ndev->stats
	ndev->stats.rx_packets++;
	ndev->stats.rx_bytes += rx_len;

	//print_hex_dump(KERN_DEBUG, "Receive Packet payload: ", DUMP_PREFIX_OFFSET, 16, 1, skb->data, skb->len, true);
	return skb;
}

static void w5500_restart(struct net_device *ndev)
{
	struct sped_net *priv = netdev_priv(ndev);

	netif_stop_queue(ndev);
	w5500_hw_reset(priv);
	w5500_hw_start(priv);
	ndev->stats.tx_errors++;
	netif_trans_update(ndev);
	netif_wake_queue(ndev);
}

static void w5500_restart_work(struct work_struct *work)
{
	struct sped_net *priv = container_of(work, struct sped_net,
					       restart_work);

	w5500_restart(priv->netdev);
}

static void w5500_tx_timeout(struct net_device *ndev, unsigned int txqueue)
{
	struct sped_net *priv = netdev_priv(ndev);
	schedule_work(&priv->restart_work);
}

static void w5500_tx_skb(struct net_device *ndev, struct sk_buff *skb)
{
	//TODO 6.7://Get the sped_net using netdev_priv
	struct sped_net *priv = netdev_priv(ndev);
	u16 offset;
	
	//print_hex_dump(KERN_DEBUG, "Packet payload: ", DUMP_PREFIX_OFFSET, 16, 1, skb->data, skb->len, true);

	/*
	 * TODO 6.8: Get the Transmit Memory Write Pointer into offset
	 * Use w5500_read16 to read SOCK_TX_WR from Socket Register
	 */ 
	offset = w5500_read16(priv, SOCK_TX_WR, BLK_SOCK_REGS);

	// TODO 6.9: Copy skb->data to Tx Buff using w5500_writebuf 
	w5500_writebuf(priv, offset, BLK_SOCK_TX_BUF, skb->data, skb->len);
	// TODO 6.10: Update TX Memory Write Pointer
	w5500_write16(priv, SOCK_TX_WR, BLK_SOCK_REGS, offset + skb->len);
	//TODO 6.11: Update the tx_bytes and tx_packets in ndev->stats
	ndev->stats.tx_bytes += skb->len;
	ndev->stats.tx_packets++;
	// TODO 6.12: Free up the skb with dev_kfree_skb
	dev_kfree_skb(skb);

	//TODO 6.13: Finally transmit the packet with SOCK_CMD_SEND using the w5500_command
	w5500_command(priv, SOCK_CMD_SEND);
}

static void w5500_tx_work(struct work_struct *work)
{
	/*
	 * TODO 6.4: Get sped_net structure from work struct
	 * use container_of macro for the same
	 * container_of(ptr, type, member)
	 */
	struct sped_net *priv = container_of(work, struct sped_net,
					       tx_work);
	// TODO 6.5: Assign the tx_skb to skb variable
	struct sk_buff *skb = priv->tx_skb;

	priv->tx_skb = NULL;

	if (WARN_ON(!skb))
		return;
	//TODO 6.6: Perform the remaining processing with w5500_tx_skb
	w5500_tx_skb(priv->netdev, skb);
}

static netdev_tx_t w5500_start_tx(struct sk_buff *skb, struct net_device *ndev)
{
	//TODO 6.3: Get the sped_net structure with netdev_priv
	struct sped_net *priv = netdev_priv(ndev);
	//TODO 6.4: Stop the Queue with netif_stop_queue
	netif_stop_queue(ndev);

	WARN_ON(priv->tx_skb);
	//TODO 6.5: Assign skb to tx_skb field of sped_net structure
	priv->tx_skb = skb;
	//TODO 6.6: Schedule the tx_work with queue_work
	//bool queue_work(struct workqueue_struct *wq, struct work_struct *work)
	queue_work(priv->xfer_wq, &priv->tx_work);

	return NETDEV_TX_OK;
}

static void w5500_rx_work(struct work_struct *work)
{
	/*
	 * TODO 7.5: Get sped_net structure from work struct
	 * use container_of macro for the same
	 * container_of(ptr, type, member)
	 */
	struct sped_net *priv = container_of(work, struct sped_net,
					       rx_work);
	struct sk_buff *skb;

	/*
	 * TODO 7.6.: Keep receiving the skb into skb in while loop
	 * using w5500_rx_skb and send it to the network stack
	 * by calling netif_rx()
	 */
	while ((skb = w5500_rx_skb(priv->netdev)))
		netif_rx(skb);

	
	// TODO 7.18: Enable Interrupts for Socket 0
	w5500_write(priv, COMN_IMR, BLK_COMN_REGS, IR_S0);
}

static irqreturn_t w5500_interrupt(int irq, void *ndev_instance)
{
	struct net_device *ndev = ndev_instance;
	struct sped_net *priv = netdev_priv(ndev);

	u8 ir;

	//TODO 6.2: Read SOCK 0 Interrupt Register into ir
	w5500_read(priv, SOCK_IR, BLK_SOCK_REGS, &ir);
	if (!ir)
		return IRQ_NONE;
	//TODO 6.3: Clear the Interrupt by ir into SOCK 0 Interrupt Register
	w5500_write(priv, SOCK_IR, BLK_SOCK_REGS, ir);
	
	//TODO 6.14: Check if IR_SENDOK is set	
	if (ir & IR_SENDOK) {
		//printk("TX Interrupt\n");
		netif_dbg(priv, tx_done, ndev, "tx done\n");
		//TODO 6.15: Wake-up the queue with netif_wake_queue(netdev)
		netif_wake_queue(ndev);
	}
	
	//TODO 7.2: Check if IR_RECV is set	
	if (ir & IR_RECV) {
		//printk("RX Interrupt\n");
		//TODO 7.3 Disable Interrupts by writing 0 into IMR Register
		w5500_write(priv, COMN_IMR, BLK_COMN_REGS, 0);
		//TODO 7.4: Queue the rx_work with queue_work(wq, work)
		queue_work(priv->xfer_wq, &priv->rx_work);
	}

	return IRQ_HANDLED;
}

static int sped_net_open(struct net_device *ndev)
{
	//TODO 5.1: Get the sped_net with netdev_priv
	struct sped_net *priv = netdev_priv(ndev);

	dev_info(&priv->spidev->dev, "sped Open\n");
	//TODO 5.2: Stop the queue with netdev_stop_queue
	//TODO 5.3: Comment out netif_stop_queue
	//netif_stop_queue(net);
	netif_info(priv, ifup, ndev, "enabling\n");
	//TODO 5.4: Start the hardware with w5500_hw_start
	w5500_hw_start(priv);
	//TODO 5.5: Start the queue with netif_start_queue
	netif_start_queue(ndev);
	//TODO 5.6: Turn the Carrier On with netif_carierr_on
	netif_carrier_on(ndev);

	return 0;
}

static int sped_net_release(struct net_device *ndev)
{
	//TODO 5.7: Get the sped_net with netdev_priv
	struct sped_net *priv = netdev_priv(ndev);

	dev_info(&priv->spidev->dev, "sped Close\n");
	
	netif_info(priv, ifdown, ndev, "shutting down\n");
	//TODO 5.8: Shut the Hardware with w5500_hw_close
	w5500_hw_close(priv);
	//TODO 5.9: Turn off the carrier
	netif_carrier_off(ndev);
	//TODO 5.10: Stop the queue with netdev_stop_queue
	netif_stop_queue(ndev);

	return 0;
}

/*
 * TODO 1.8: Populate net_device interface operations
 * ndo_open, ndo_stop, nod_start_xmit and ndo_tx_timeout
 */
static const struct net_device_ops sped_net_ops = {
	.ndo_open = sped_net_open,
	.ndo_stop = sped_net_release,
	.ndo_start_xmit	= w5500_start_tx,
	.ndo_tx_timeout	= w5500_tx_timeout,
};

static void sped_net_init(struct net_device *net)
{
	//TODO 1.9: Extract the sped_net struct using netdev_priv and assign 
	// it to priv variable
	struct sped_net *priv = netdev_priv(net);
	// It would initialize all the fields of net_device
	pr_info("Network Device Initialize\n");
	// TODO 1.10: Initialize the net_device using ether_setup
	ether_setup(net);
	// TODO 1.11: Initialize the netdev_ops field with sped_net_ops
	net->netdev_ops = &sped_net_ops;
	//TODO 1.12: Clear the sped_net struture priv
	memset(priv, 0, sizeof (struct sped_net));
	//TODO 1.13: Assign the initialized netdev struct to netdev field of sped_net
	priv->netdev = net;
}

static int sped_probe(struct spi_device *spi)
{
	struct sped_net *priv;
	struct net_device *netdev;
	int status;
	const void *mac = NULL;
	u8 tmpmac[ETH_ALEN];

	dev_info(&spi->dev, "Probe function Invoked\n");
	/* 
	 * TODO 2.1: Get the MAC address from dtb tmpmac variable
	 * use int of_get_mac_address(struct device_node *np, u8 *mac)
	 */
	status = of_get_mac_address(spi->dev.of_node, tmpmac);
	if (!status)
		mac = tmpmac;
	/*
	 * TODO 1.3: Allocate the net_device structure with alloc_netdev
     * Reserve the space of struct sped_net and name the interface as sped
	 * Register sped_net_init as setup function
	 */ 
	netdev = alloc_netdev(sizeof(struct sped_net), "sped%d", NET_NAME_UNKNOWN, sped_net_init);
	if (!netdev) {
		printk("Alloc netdev failed\n");
		return -ENOMEM;
	}
	/*
	 * TODO 2.2: Connect the net_device to the physical SPI device
	 * Use SET_NETDEV_DEV(netdev, parent_dev) for the same
	 * It ensures the network interface appears under the correct physical 
	 * hardware in the /sys/devices/
	 */
	SET_NETDEV_DEV(netdev, &spi->dev);
	/*
	 * TODO 1.4A: Attach allocated net_device structure to driver_data
	 * field of spi->dev by using void dev_set_drvdata(struct device *dev, void *data);
	 * Will be used in remove function to get hold net_device structure for cleanup
	 */
	dev_set_drvdata(&spi->dev, netdev);
	/* 
	 * 1.4B: Retreive the allocate sped_net structure from net_device structure
	 * Use netdev_priv from the same
	 */
	priv = netdev_priv(netdev);

	priv->spidev = spi;
	mutex_init(&priv->lock);

	//TODO 1.5: Register net_device with register_netdev
	status = register_netdev(netdev);
	if (status < 0)
		goto err_register;
	/*
	 * TODO 3.: Allocate workqueue using the API
	 * struct workqueue_struct *alloc_workqueue(const char *fmt, 
	 *		unsigned int flags, int max_active, ...);
	 * 		use flags WQ_MEM_RECLAIM, and name as netdev_name(netdev)
	 */
	priv->xfer_wq = alloc_workqueue("%s", WQ_MEM_RECLAIM, 0,
					netdev_name(netdev));
	if (!priv->xfer_wq) {
		status = -ENOMEM;
		goto err_wq;
	}
	/* 
	 * TODO 6.2: Initialize the work for tx path
	 * Use INIT_WORK(_work, _func)
	 */
	INIT_WORK(&priv->tx_work, w5500_tx_work);
	/* 
	 * TODO 7.1: Initialize the work for rx path
	 * Use INIT_WORK(_work, _func)
	 */
	INIT_WORK(&priv->rx_work, w5500_rx_work);
	/* Scheduled during .ndo_tx_timeout callback handler */
	INIT_WORK(&priv->restart_work, w5500_restart_work);

	/*
	 * TODO 1.6 : if mac is not NULL, 
	 * use void eth_hw_addr_set(struct net_device *dev, const u8 *addr) to set
	 * the mac address for the interface
 	 * else use void eth_hw_addr_random(struct net_device *dev)
	 */
	if (mac)
		eth_hw_addr_set(netdev, mac);
	else
		eth_hw_addr_random(netdev);
	// TODO 3.1A: Reset the hardware w5500_hw_reset
	status = w5500_hw_reset(priv);
	if (status)
		goto err_hw;
	/*
	 * Ignore 1.7. Not Required
	 * TODO 1.7: Register sped_irq as interrupt handler with request_irq
	 * Use IRQF_TRIGGER_LOW | IRQF_ONESHOT as flags and pass netdev as data
	 * use netdev_name to name the lable the irq registeration
	*/ 
	/* TODO 6.1: Register w5500_interrupt as bottom half (no top half)
	 * Use IRQF_TRIGGER_LOW | IRQF_ONESHOT as flags and pass netdev as data
	 * use netdev_name to name the lable the irq registeration
	 * use request_threaded_irq(irq_no, top_half, bottom_half, flags, name, data)
	 */
	status = request_threaded_irq(spi->irq, NULL, w5500_interrupt,
					   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
					   netdev_name(netdev), netdev);
	if (status)
		goto err_hw;

	return 0;

err_hw:
	destroy_workqueue(priv->xfer_wq);
err_wq:
	unregister_netdev(netdev);
err_register:
	free_netdev(netdev);
	return status;
}

static void sped_remove(struct spi_device *spi)
{
	/*
     * TODO 1.14: Get the net_device structure using dev_get_drvdata
	 * and assign it to ndev
	 */
	struct net_device *ndev = dev_get_drvdata(&spi->dev);
	/*
	 * TODO 1.15: Get the sped_net structure using netdev_priv and assign it 
	 * to priv variable
	 */
	struct sped_net *priv = netdev_priv(ndev);

	/* TODO 3.1B: Reset the hardware with w5500_hw_reset */
	w5500_hw_reset(priv);
	/* 
	 * TODO 7.19: Release the IRQ with free_irq
	 * void *free_irq(unsigned int irq, void *data);
	 */
	free_irq(spi->irq, ndev);
	// TODO Flush restart work
	flush_work(&priv->restart_work);
	//TODO 7.20: Destroy the work queue
	destroy_workqueue(priv->xfer_wq);
	//TODO 1.17: Unregister the net_device structure with unregister_netdev
	unregister_netdev(ndev);
	//TODO 1.18: Free up the net_device using free_netdev
	free_netdev(ndev);
}

//TODO 1.1: Populate the sped_dt_ids with compatible property
static const struct of_device_id sped_dt_ids[] = {
        { .compatible = "wiznet,w5500" },
        { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sped_dt_ids);

//TODO 1.2: Populate .of_match_table (in .driver), .probe, .remove
static struct spi_driver sped_driver = {
        .driver = {
                .name = "sped",
                .of_match_table = sped_dt_ids,
         },
        .probe = sped_probe,
        .remove = sped_remove,
};
module_spi_driver(sped_driver);

MODULE_DESCRIPTION("Simple SPI Ethernet device network driver for W5500");
MODULE_AUTHOR("Embitude Information Technologies");
MODULE_LICENSE("GPL");
