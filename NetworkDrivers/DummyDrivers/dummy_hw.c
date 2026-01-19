#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/icmp.h>

int32_t (*dummy_drv_rx)(struct sk_buff *);

struct task_struct *hw_thread_id;
struct sk_buff_head skb_tx_q;

static void dummy_hw_send_irq(void)
{
	//TODO 5: Dequeue the skb (skb_dequeue) and get that into the skb variable
	struct sk_buff *skb;

	if (skb == NULL) {
		printk(KERN_ERR "DHW: unable to dequeue TX skb\n");
		return;
	}
	
	printk(KERN_ERR "DHW: sending RX interrupt for skb=%p\n", skb);
	if (dummy_drv_rx == NULL) {
		printk(KERN_ERR "DHW: Interrupt not requested, freeing\n");
		dev_kfree_skb(skb);
	} else {
		//TODO 6:  Invoke dummy rx function
	}
}

static int dummy_hw_thread(void *arg)
{
	printk(KERN_INFO "DHW thread started: %p ..\n", current);
	
	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		
		msleep(10);
		
		/* Do RX here */
		//TODO 4: Generate the irq (using dummy_hw_send_irq), if skb queue is not empty
	}
	printk(KERN_INFO "DHW thread closed\n");
	return 0;
}

int32_t lt_request_irq(bool mode, int32_t (*rx_fn)(struct sk_buff *skb))
{
	if ((mode == true) && (dummy_drv_rx == NULL)) {
		printk(KERN_ERR "DHW: Register IRQ for RX\n");
		dummy_drv_rx = rx_fn;
	} else if((mode == false) && (dummy_drv_rx != NULL)) {
		printk(KERN_ERR "DHW: Deregister IRQ for RX\n");
		/* Cleanup the TX Q */
		wake_up_process(hw_thread_id);
	}
	return 0;
}
EXPORT_SYMBOL(lt_request_irq);

int lt_hw_tx(struct sk_buff *skb)
{
	printk(KERN_INFO "Dummy: %s() - called\n", __func__);
	//TODO 3: Insert the skb at the tail of queue
	return 0;
}
EXPORT_SYMBOL(lt_hw_tx);

static int init_hw(void)
{	
	printk(KERN_INFO "Dummy Eth Module Init\n");
	
	//TODO 1: Initialize the skb queue with skb_queue_head_init
	//TODO 2: Create a thread which executes dummy_hw_thread function
	if (hw_thread_id) {
		printk("Thread Created successfully! Waking Up");
	}
	else
		printk(KERN_INFO "Thread creation failed\n");
	return 0;
}

static void deinit_hw(void)
{	
	int err;
	if (hw_thread_id != NULL)
	{
		err = kthread_stop(hw_thread_id);
		if(!err)
			printk(KERN_INFO "Thread stopped");
	}
}

module_init(init_hw);
module_exit(deinit_hw);
MODULE_LICENSE("GPL");
