#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int irq = 10; // Global variable with default value
static char *devname = "mydevice";
static int addr[4] = { 0 };
static int count;

// Macro for an integer parameter: name, type, permissions
module_param(irq, int, 0660);
MODULE_PARM_DESC(irq, "The IRQ number for the device"); // Documentation macro

// Macro for a string parameter: name, type (charp), permissions
module_param(devname, charp, 0660);
MODULE_PARM_DESC(devname, "The name of the device");

// Macro for an array parameter: name, type, pointer to count, permissions
module_param_array(addr, int, &count, 0660);
MODULE_PARM_DESC(addr, "Base addresses (array of int)");

static int __init simple_init(void)
{
    printk(KERN_INFO "Module loaded: irq=%d, name=%s, count=%d\n", irq, devname, count);
    return 0;
}

static void __exit simple_cleanup(void)
{
    printk(KERN_INFO "Module unloaded\n");
}

module_init(simple_init);
module_exit(simple_cleanup);
MODULE_LICENSE("GPL");
