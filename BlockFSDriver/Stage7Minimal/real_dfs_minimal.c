/* Real Simula File System Module */
#include <linux/module.h> /* For module related macros, ... */
#include <linux/kernel.h> /* For printk, ... */
#include <linux/version.h> /* For LINUX_VERSION_CODE & KERNEL_VERSION */
#include <linux/fs.h> /* For system calls, structures, ... */
#include <linux/errno.h> /* For error codes */
#include <linux/slab.h> /* For kzalloc, ... */

#include "real_dfs_ds.h" /* For SFS related defines, data structures, ... */
#include "real_dfs_ops.h" /* For SFS related operations */

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
static int get_bit_pos(unsigned int val)
{
	int i;

	for (i = 0; val; i++)
	{   
		val >>= 1;
	}   
	return (i - 1); 
}
static int dfs_fill_super(struct super_block *sb, void *data, int silent)
{
	dfs_info_t *info;

	printk(KERN_INFO "dfs: dfs_fill_super\n");
	if (!(info = (dfs_info_t *)(kzalloc(sizeof(dfs_info_t), GFP_KERNEL))))
		return -ENOMEM;
	info->vfs_sb = sb;
	if (init_browsing(info) < 0)
	{
		kfree(info);
		return -EIO;
	}
	/* Updating the VFS super_block */
	sb->s_magic = 0; /* TODO 12: File System Type extracted from our superblock */
	sb->s_blocksize = 0; /* TODO 13A: File System Block Size extracted from our superblock */
	sb->s_blocksize_bits = get_bit_pos(0 /* TODO 13B: File System Block Size */);
	sb->s_type = NULL; // file_system_type
	sb->s_op = NULL; // super block operations

	// Obtain an inode from VFS
	dfs_root_inode = iget_locked(sb, 0 /* TODO 14: Root Inode Number as per our translation logic*/);
	if (!dfs_root_inode)
	{
		shut_browsing(info);
		kfree(info);
		return -EACCES;
	}
	if (dfs_root_inode->i_state & I_NEW) // allocated fresh now
	{
		printk(KERN_INFO "dfs: Got new root inode, let's fill in\n");
		dfs_root_inode->i_op = &dfs_iops; // inode operations
		dfs_root_inode->i_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		dfs_root_inode->i_fop = &dfs_fops; // file operations
		dfs_root_inode->i_mapping->a_ops = &dfs_aops; // address operations
		unlock_new_inode(dfs_root_inode);
	}
	else
	{
		printk(KERN_INFO "dfs: Got root inode from inode cache\n");
	}

	sb->s_root = d_make_root(dfs_root_inode);
	if (!sb->s_root)
	{
		iget_failed(dfs_root_inode);
		shut_browsing(info);
		kfree(info);
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
	return mount_bdev(fs_type, flags, devname, data, &dfs_fill_super);
}

static void dfs_kill_sb(struct super_block *sb)
{
	dfs_info_t *info = (dfs_info_t *)(sb->s_fs_info);

	kill_block_super(sb);
	if (info)
	{
		shut_browsing(info);
		kfree(info);
	}
}

static struct file_system_type dfs =
{
	name: "dfs", /* Name of our file system */
	kill_sb: dfs_kill_sb,
	owner: THIS_MODULE
};

static int __init dfs_init(void)
{
	int err;

	err = register_filesystem(&dfs);
	return err;
}

static void __exit dfs_exit(void)
{
	unregister_filesystem(&dfs);
}

module_init(dfs_init);
module_exit(dfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("File System Module for real Demo File System");
