#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/of_net.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

struct sped_net {
	struct spi_device *spidev;
	struct net_device *netdev;
};

static irqreturn_t sped_irq(int irq, void *irq_data)
{
	struct sped_net *priv = (struct sped_net *) irq_data;
	dev_info(&priv->spidev->dev, "IRQ occured!\n");
	return 0;
}

static int sped_probe(struct spi_device *spi)
{
	int status;
	struct sped_net *priv;

	dev_info(&spi->dev, "Probe function Invoked\n");
	/*
	 *TODO 1.3: Allocate the sped_net structure using kzalloc
	 * and assign it to priv
	 */
	priv->spidev = spi;

	/*
	 * TODO 1.4: Attach a pointer to driver's custom data structure
	 * using spi_set_drvdata(spi_device, void *)
	 */

	/*
	 * TODO 1.5: Register Irq handler using request_irq.
	 * request_irq(irq_no, handler, flags, name, data)
	 * spi-irq contains the irq number. Pass priv as data
	 */
	if (status) {
		kfree(priv);
		return status;
	}
	
	/* 
	 * TODO 2.1: Read the Retry Time-value Register. Default value is 2000 (0x07D0)
	 * Form the command in wr_buf and read back (2 bytes) in rd_buf
	 * use spi_write_then_read(spi_device, wr_buf, wr_len, rd_buf, rd_len)
	 */
	u8 wr_buf[3] = {0x00, 0X00, 0x00};
	u8 rd_buf[4];
	printk("buf[0] = %x, buf[1] = %x\n", rd_buf[0], rd_buf[1]);
	return 0;
}

static void sped_remove(struct spi_device *spi)
{
	struct sped_net *priv;

	dev_info(&spi->dev, "Remove function\n");
	/*
	 * TODO 1.6: Get the driver data into priv using spi_get_drvdata
	 */
	/*
	 * TODO 1.7: Free Irq number by using free_irq(irq_no, data)
	 */

	//TODO 1.8: De-allocate the private data with kfree
}

//TODO 1.1: Populate the sped_dt_ids with compatible property
static const struct of_device_id sped_dt_ids[] = {
        {},
        { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sped_dt_ids);

//TODO 1.2: Populate .of_match_table (in .driver), .probe, .remove
static struct spi_driver sped_driver = {
        .driver = {
                .name = "sped",
         },
};
module_spi_driver(sped_driver);

MODULE_DESCRIPTION("Simple SPI Ethernet device network driver for W5500");
MODULE_AUTHOR("Embitude Information Technologies");
MODULE_LICENSE("GPL");
