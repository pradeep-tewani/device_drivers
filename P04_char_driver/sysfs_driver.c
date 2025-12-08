#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h> // Required for copy_to_user/copy_from_user

#define DRIVER_NAME "sysfs_char_dev"
#define DEVICE_COUNT 1

static dev_t dev_num;
static struct class *dev_class;
static struct cdev char_cdev;
static struct device *dev_device;

/* A variable we want to expose to user space via sysfs */
static int driver_value = 0;

/* 
 * The 'show' function is called when the sysfs file is read (e.g., cat).
 * It copies the kernel variable value into the user buffer.
 */
static ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// TODO 3: Copy the Kernel variable into the user buffer
    return 0;
}

/* 
 * The 'store' function is called when the sysfs file is written to (e.g., echo).
 * It reads the user input from the buffer and updates the kernel variable.
 */
static ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	// TODO 4: Update the Kernel variable with received value
    return count;
}

/* 
 * TODO 1: Define the device attribute using a macro: 
 * DEVICE_ATTR(name, permissions, show function, store function) 
 */

/* Standard file operations for the /dev/ entry (optional for this example) */
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    /* .read = ... */
    /* .write = ... */
    /* Add open, release, etc., if needed for standard char device behavior */
};

/* Module initialization function */
static int __init sysfs_char_init(void)
{
    int ret;

    /* 1. Allocate major/minor numbers dynamically */
    ret = alloc_chrdev_region(&dev_num, 0, DEVICE_COUNT, DRIVER_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate character device region\n");
        return ret;
    }

    /* 2. Create a sysfs class (creates /sys/class/sysfs_char_dev) */
    dev_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(dev_class)) {
        printk(KERN_ERR "Failed to create device class\n");
        unregister_chrdev_region(dev_num, DEVICE_COUNT);
        return PTR_ERR(dev_class);
    }

    /* 3. Create the device node and register with sysfs */
    /* This creates /dev/sysfs_char_dev-0 and a directory in /sys/class/sysfs_char_dev/ */
    dev_device = device_create(dev_class, NULL, dev_num, NULL, DRIVER_NAME "-%d", 0);
    if (IS_ERR(dev_device)) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(dev_class);
        unregister_chrdev_region(dev_num, DEVICE_COUNT);
        return PTR_ERR(dev_device);
    }

    /* 
     * 4. Create the custom sysfs file within the device's sysfs directory 
     * TODO 2: Create the custom sysfs file within the device's sysfs directory 
	 * Use the API device_create_file
     */
    if (ret < 0) {
        printk(KERN_ERR "Failed to create sysfs attribute file\n");
        device_destroy(dev_class, dev_num);
        class_destroy(dev_class);
        unregister_chrdev_region(dev_num, DEVICE_COUNT);
        return ret;
    }

    /* 5. Initialize and add the character device structure (if standard fops are used) */
    cdev_init(&char_cdev, &fops);
    char_cdev.owner = THIS_MODULE;
    cdev_add(&char_cdev, dev_num, DEVICE_COUNT);

    printk(KERN_INFO DRIVER_NAME ": Module loaded, device file created in /dev and sysfs\n");
    return 0;
}

/* Module exit function */
static void __exit sysfs_char_exit(void)
{
    cdev_del(&char_cdev);
	/* TODO 5: Remove the Custom sysfs file by using device_remove_file */
    device_destroy(dev_class, dev_num); /* Destroy the /dev node and sysfs directory */
    class_destroy(dev_class);
    unregister_chrdev_region(dev_num, DEVICE_COUNT);
    printk(KERN_INFO DRIVER_NAME ": Module unloaded\n");
}

module_init(sysfs_char_init);
module_exit(sysfs_char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("A character driver with sysfs interaction");
