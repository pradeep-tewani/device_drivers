#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/timer.h>

static struct timer_list panic_timer;

static void do_panic(struct timer_list *unused)
{
    *(int*)0x42 = 'a';
}

static int my_panic_init(void) { 

    printk("Panic from the module\n"); 

	timer_setup(&panic_timer,  do_panic, 0);
    mod_timer(&panic_timer, jiffies + 2 * HZ);

	return (0); 
} 

static void __exit my_panic_exit(void) { 
        printk("Goodbye world\n"); 
} 
 
module_init(my_panic_init); 
module_exit(my_panic_exit);
MODULE_LICENSE("GPL");
