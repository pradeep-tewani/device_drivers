/* Real Simula File System Module */
#include <linux/module.h> /* For module related macros, ... */
#include <linux/kernel.h> /* For printk, ... */
#include <linux/version.h> /* For LINUX_VERSION_CODE & KERNEL_VERSION */
#include <linux/fs.h> /* For system calls, structures, ... */
#include <linux/errno.h> /* For error codes */

#include "real_dfs_ds.h" /* For SFS related defines, data structures, ... */

/*
 * Data declarations
 */
static struct file_system_type dfs;
static struct super_operations dfs_sops;
static struct inode_operations dfs_iops;
static struct file_operations dfs_fops;
static struct address_space_operations dfs_aops;

static struct inode *dfs_root_inode;

/*
 * File-System Supporting Operations
 */
static int dfs_fill_super(struct super_block *sb, void *data, int silent)
{
	printk(KERN_INFO "dfs: dfs_fill_super\n");

	sb->s_blocksize = 0 /* TODO 5: Block size for this file system */;
	sb->s_blocksize_bits = 0 /* TODO 6: log2 of block size for this file system */;
	sb->s_magic = 0 /* TODO 7: File system type */;
	sb->s_type = 0; // file_system_type
	sb->s_op = 0; // super block operations

	// Obtain an inode from VFS
	dfs_root_inode = iget_locked(sb, 0/* TODO 8: Inode number for the root of this file system */);
	if (!dfs_root_inode)
	{
		return -EACCES;
	}
	if (dfs_root_inode->i_state & I_NEW) // allocated fresh now
	{
		printk(KERN_INFO "dfs: Got new root inode, let's fill in\n");
		dfs_root_inode->i_op = &dfs_iops; // inode operations
		dfs_root_inode->i_mode = S_IFDIR | S_IRWXU |
			S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		dfs_root_inode->i_fop = &dfs_fops; // file operations
		dfs_root_inode->i_mapping->a_ops = &dfs_aops; // address operations
		unlock_new_inode(dfs_root_inode);
	}
	else
	{
		printk(KERN_INFO "dfs: Got root inode from inode cache\n");
	}

	sb->s_root = d_make_root(NULL /* TODO 9: Root's VFS inode to be attached to VFS superblock */);
	if (!sb->s_root)
	{
		iget_failed(dfs_root_inode);
		return -ENOMEM;
	}

	return 0;
}

/*
 * File-System Operations
 */
static struct dentry *dfs_mount(struct file_system_type *fs_type, int flags, const char *devname, void *data)
{
	printk(KERN_INFO "dfs: devname = %s\n", devname);

	 /* dfs_fill_super this will be called to fill the super block */
	return mount_bdev(fs_type, flags, devname, data, NULL /* TODO 4: Callback handler to populate the super block */);
}

/* 
 * TODO 1: Populate the file_system_type structure 
 * Populate the fields - name & mount
 */
static struct file_system_type dfs =
{
	/* Name of our file system */
	kill_sb: kill_block_super,
	owner: THIS_MODULE
};

static int __init dfs_init(void)
{
	int err;
	/* TODO 2: Register the filesytem with register_filesystem(struct file_system_type *) */
	return err;
}

static void __exit dfs_exit(void)
{
	/* TODO 3: Unregister the filesytem */
}

module_init(dfs_init);
module_exit(dfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("File System Module for real Demo File System");
