#ifndef W5500_H
#define W5500_H

#include <linux/netdevice.h>

#define BLK_COMN_REGS		0x00
#define BLK_SOCK_REGS		0x01
#define BLK_SOCK_TX_BUF		0x02
#define BLK_SOCK_RX_BUF		0x03

#define COMN_MR		0x0000 /* Mode Register */
#define COMN_MR_RST	0x80 /* S/W reset */
#define COMN_MR_PB	0x10 /* Ping block */
#define COMN_MR_AI	0x02 /* Address Auto-Increment */
#define COMN_MR_IND	0x01 /* Indirect mode */
#define COMN_SHAR	0x0009 /* Source MAC address */
#define COMN_IR		0x0015 /* Interrupt Register */
#define COMN_IMR	0x0018 /* Interrupt Mask Register */
#define IR_S0		0x01 /* S0 interrupt */
#define COMN_RTR		0x0019 /* Interrupt Mask Register */
#define W5100_COMMON_REGS_LEN	0x0040
#define SOCK_CMD_OPEN	0x01
#define SOCK_CMD_CLOSE	0x10
#define SOCK_CMD_SEND	0x20
#define SOCK_CMD_RECV	0x40

#define SOCK_MR		0x0000 /* Sn Mode Register */
#define SOCK_CR		0x0001 /* Sn Command Register */
#define SOCK_IR		0x0002 /* Sn Interrupt Register */
#define IR_SENDOK	0x10
#define IR_RECV		0x04
#define SOCK_SR		0x0003 /* Sn Status Register */
#define SOCK_TX_FSR		0x0020 /* Sn Transmit free memory size */
#define SOCK_TX_RD		0x0022 /* Sn Transmit memory read pointer */
#define SOCK_TX_WR		0x0024 /* Sn Transmit memory write pointer */
#define SOCK_RX_RSR		0x0026 /* Sn Receive free memory size */
#define SOCK_RX_RD		0x0028 /* Sn Receive memory read pointer */

#define SOCK_RX_BUF_SZ	0x001E
#define SOCK_TX_BUF_SZ	0x001F
#define SOCK_TX_FSR		0x0020 /* Sn Transmit free memory size */
#define SOCK_TX_RD		0x0022 /* Sn Transmit memory read pointer */
#define SOCK_TX_WR		0x0024 /* Sn Transmit memory write pointer */
#define SOCK_RX_RSR		0x0026 /* Sn Receive free memory size */
#define SOCK_RX_RD		0x0028 /* Sn Receive memory read pointer */
#define SOCK_MR_MACRAW	0x04
#define SOCK_MR_MF		0x80

#define CTL_WRITE	BIT(2)

#define RX_MEM_SIZE		0x04000
#define TX_MEM_SIZE		0x04000

struct sped_net {
	struct spi_device *spidev;
	struct mutex lock;
	struct net_device *netdev;
	struct napi_struct napi;
	bool promisc;
	u32 msg_enable;

	struct workqueue_struct *xfer_wq;
	struct work_struct rx_work;
	struct sk_buff *tx_skb;
	struct work_struct tx_work;
	struct work_struct setrx_work;
	struct work_struct restart_work;
	
	u8 cmd_buf[3] ____cacheline_aligned;
};

int w5500_command(struct sped_net *priv, u8 cmd);
int w5500_write(struct sped_net *priv, u16 offset, u8 block, u8 buf); 
int w5500_read(struct sped_net *priv, u16 offset, u8 block, u8 *data); 
int w5500_read16(struct sped_net *priv, u16 offset, u8 block);
int w5500_write16(struct sped_net *priv, u16 offset, u8 block, u16 data);
int w5500_readbulk(struct sped_net *priv, u16 offset, u8 block, u8 *buf, int len);
int w5500_writebulk(struct sped_net *priv, u16 offset, u8 block, u8 *buf, int len);
int w5500_reset(struct sped_net *priv);

#endif
