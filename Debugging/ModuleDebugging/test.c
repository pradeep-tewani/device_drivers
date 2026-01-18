#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
 
static int my_oops_init(void) { 

	int *p;
	
    printk("oops from the module\n"); 
	*p = 0; 
	return (0); 
} 

static void __exit my_oops_exit(void) { 
        printk("Goodbye world\n"); 
} 
 
module_init(my_oops_init); 
module_exit(my_oops_exit);
MODULE_LICENSE("GPL");
