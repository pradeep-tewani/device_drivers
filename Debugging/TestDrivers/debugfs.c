#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>

// This directory entry will point to `/sys/kernel/debug/example1`.
static struct dentry *dir = 0;

// File `/sys/kernel/debug/example1/hello` points to this variable.
static u32 hello = 0;

// This is called when the module loads.
int init_module(void)
{
    // Create directory `/sys/kernel/debug/example1`.
    dir = debugfs_create_dir("example1", 0);
    if (!dir) {
        // Abort module load.
        printk(KERN_ALERT "debugfs_example1: failed to create /sys/kernel/debug/example1\n");
        return -1;
    }

    // Create file `/sys/kernel/debug/example1/hello`.
    // Read/write operations on the file result in read/write operations on the variable `hello`.
    //
    debugfs_create_u32("hello", 0666, dir, &hello);

    return 0;
}

// This is called when the module is removed.
void cleanup_module(void)
{
    // We must manually remove the debugfs entries we created. They are not
    // automatically removed upon module removal.
    debugfs_remove_recursive(dir);
}

MODULE_LICENSE("GPL");
