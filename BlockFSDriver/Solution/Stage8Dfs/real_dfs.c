/* Real Simula File System Module */
#include <linux/module.h> /* For module related macros, ... */
#include <linux/kernel.h> /* For printk, ... */
#include <linux/version.h> /* For LINUX_VERSION_CODE & KERNEL_VERSION */
#include <linux/fs.h> /* For system calls, structures, ... */
#include <linux/errno.h> /* For error codes */
#include <linux/blkdev.h> /* struct gendisk */
#include <linux/slab.h> /* For kzalloc, ... */
#include <linux/buffer_head.h> /* map_bh, block_write_begin, block_write_full_page, generic_write_end, ... */
#include <linux/blkdev.h> /* block_size, ... */
#include <linux/mpage.h> /* mpage_readpage, ... */

#include "real_dfs_ds.h" /* For DFS related defines, data structures, ... */
#include "real_dfs_ops.h" /* For DFS related operations */

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
 * Address Space Operations
 */
static int dfs_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create)
{
	struct super_block *sb = inode->i_sb;
	dfs_info_t *info = (dfs_info_t *)(sb->s_fs_info);
	dfs_file_entry_t fe;
	sector_t phys;
	int retval;

	printk(KERN_INFO "dfs: dfs_get_block called for I: %ld, B: %llu, C: %d\n",
		inode->i_ino, (unsigned long long)(iblock), create);

	if (iblock >= DEMO_FS_DATA_BLOCK_CNT /* TODO 10: Compare with the max data block count we support in DFS design */)
	{
		return -ENOSPC;
	}
	if ((retval = dfs_get_file_entry(info, inode->i_ino, &fe)) < 0)
	{
		return retval;
	}
	if (!fe.blocks[iblock])
	{
		if (!create)
		{
			return -EIO;
		}
		else
		{
			if ((fe.blocks[iblock] = dfs_get_data_block(info)/* TODO 11: Get a free block for data block from DFS */) == INV_BLOCK)
			{
				return -ENOSPC;
			}
			if ((retval = dfs_update_file_entry(info, inode->i_ino, &fe)) < 0) /* TODO 12: Update the file entry in the filesystem */
			{
				return retval;
			}
		}
	}
	/* For simplicity of TODO 13, you may assume that DFS block size is multiple of block driver block size */
	//phys = (fe.blocks[iblock] * (info->sb.block_size)) / block_size(info->vfs_sb->s_bdev);/* TODO 13: Translate the DFS block number fe.blocks[iblock] to block driver block number */;
	phys = fe.blocks[iblock];
	map_bh(bh_result, sb, phys);

	return 0;
}
//static int dfs_readpage(struct file *file, struct page *page)
static void dfs_readpage(struct readahead_control *rac)
{
	printk(KERN_INFO "dfs: dfs_readpage\n");
	//return mpage_readpage(page, NULL/* TODO 14A: Callback function to get the block number of the desired data block */);
	//mpage_readahead(rac, dfs_get_block /* TODO 14A: Callback function to get the block number of the desired data block */);
	//return 0;
}

static int dfs_read_folio(struct file *file, struct folio *folio)
{
	return block_read_full_folio(folio, dfs_get_block);
}

static int dfs_write_begin(struct file *file, struct address_space *mapping, 
	loff_t pos, unsigned int len, struct page **pagep, void **fsdata)
//static int dfs_write_begin(struct file *file, struct address_space *mapping,
//	loff_t pos, unsigned len, unsigned flags, struct page **pagep, void **fsdata)
{
	printk(KERN_INFO "dfs: dfs_write_begin\n");
	*pagep = NULL;
	//return block_write_begin(mapping, pos, len, flags, pagep, NULL/* TODO 14B: Callback function to get the block number of the desired data block */);
	return block_write_begin(mapping, pos, len, pagep, dfs_get_block /* TODO 14B: Callback function to get the block number of the desired data block */);
}
#if 0
static int dfs_writepage(struct page *page, struct writeback_control *wbc)
{
	printk(KERN_INFO "dfs: dfs_writepage\n");
	return block_write_full_page(page, NULL /* TODO 14C: Callback function to get the block number of the desired data block */, wbc);
}
#endif

static int dfs_writepages(struct address_space *mapping,
     			struct writeback_control *wbc)
{
	printk(KERN_INFO "dfs: dfs_writepages\n");
	return mpage_writepages(mapping, wbc, dfs_get_block);
}
/* 
 * TODO 20: Populate the address space operations
 * readpage, write_begin, writepage and write_end
 *
 */
static struct address_space_operations dfs_aops =
{
	.write_begin = dfs_write_begin,
	.writepages = dfs_writepages,
	.write_end = generic_write_end,
	//.readahead = dfs_readpage,
	.read_folio = dfs_read_folio,
};

/*
 * File Operations
 */
static int dfs_file_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "dfs: dfs_file_release\n");
	return 0;
}
static int dfs_iterate(struct file *file, struct dir_context *ctx)
{
	dfs_info_t *info = file_inode(file)->i_sb->s_fs_info;

	printk(KERN_INFO "dfs: dfs_iterate: %Ld\n", ctx->pos);

	if (!dir_emit_dots(file, ctx))
	{
		return -ENOSPC;
	}
	return dfs_list(info, file, ctx); /* TODO 6: Fill in all the parameters */
}
static struct file_operations dfs_fops =
{
	open: generic_file_open,
	release: dfs_file_release,
	read_iter: generic_file_read_iter,
	write_iter: generic_file_write_iter,
	fsync: noop_fsync
};
/* TODO 19: Populate the directory entry operations - iterate */
static struct file_operations dfs_dops =
{
	iterate_shared: dfs_iterate, 
};

/*
 * Inode Operations
 */
static struct dentry *dfs_inode_lookup(struct inode *parent_inode, struct dentry *dentry, unsigned int flags)
{
	dfs_info_t *info = (dfs_info_t *)(parent_inode->i_sb->s_fs_info);
	char fn[DEMO_FS_FILENAME_LEN + 1];
	int ino;
	dfs_file_entry_t fe;
	struct inode *file_inode = NULL;

	printk(KERN_INFO "dfs: dfs_inode_lookup\n");

	if (parent_inode->i_ino != dfs_root_inode->i_ino)
		return ERR_PTR(-ENOENT);
	strncpy(fn, dentry->d_name.name, DEMO_FS_FILENAME_LEN);
	fn[DEMO_FS_FILENAME_LEN] = 0;
	if ((ino = dfs_lookup(info, fn, &fe)) == INV_INODE) /* TODO 2: Fill in all the parameters */
	  return d_splice_alias(file_inode, dentry); // Possibly create a new one

	printk(KERN_INFO "dfs: Getting an existing inode\n");
	file_inode = iget_locked(parent_inode->i_sb, ino);
	if (!file_inode)
		return ERR_PTR(-EACCES);
	if (file_inode->i_state & I_NEW)
	{
		printk(KERN_INFO "dfs: Got new VFS inode for #%d, let's fill in\n", ino);
		file_inode->i_size = fe.size;
		file_inode->i_mode = S_IFREG;
		file_inode->i_mode |= ((fe.perms & 4) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
		file_inode->i_mode |= ((fe.perms & 2) ? S_IWUSR | S_IWGRP | S_IWOTH : 0);
		file_inode->i_mode |= ((fe.perms & 1) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
		//file_inode->i_atime.tv_sec = file_inode->i_mtime.tv_sec = file_inode->i_ctime.tv_sec = ktime_get_real_seconds(); /* TODO 3: DFS file timestamp */;
		//file_inode->i_atime.tv_nnsec = file_inode->i_mtime.tv_nsec = file_inode->i_ctime.tv_nsec = 0;
		file_inode->i_atime_sec = file_inode->i_mtime_sec = file_inode->i_ctime_sec = ktime_get_real_seconds(); /* TODO 3: DFS file timestamp */;
		file_inode->i_atime_nsec = file_inode->i_mtime_nsec = file_inode->i_ctime_nsec = 0;
		file_inode->i_mapping->a_ops = &dfs_aops; /* TODO 4: Assign the address ops */
		file_inode->i_fop = &dfs_fops; /* TODO 5: Assign the file ops */
		unlock_new_inode(file_inode);
	}
	else
	{
		printk(KERN_INFO "dfs: Got VFS inode from inode cache\n");
	}
	d_add(dentry, file_inode);
	return NULL;
	// Above 2 lines can be replaced by 'return d_splice_alias(file_inode, dentry);'
}
static int dfs_inode_create(struct mnt_idmap *idmap, struct inode *parent_inode, 
				struct dentry *dentry, umode_t mode,  bool excl)
//static int dfs_inode_create(struct inode *parent_inode, struct dentry *dentry, umode_t mode, bool excl)
{
	char fn[DEMO_FS_FILENAME_LEN + 1];
	int perms = 0;
	dfs_info_t *info = (dfs_info_t *)(parent_inode->i_sb->s_fs_info);
	int ino;
	struct inode *file_inode;
	dfs_file_entry_t fe;

	printk(KERN_INFO "dfs: dfs_inode_create\n");

	strncpy(fn, dentry->d_name.name, DEMO_FS_FILENAME_LEN);
	fn[DEMO_FS_FILENAME_LEN] = 0;
	if (mode & (S_IRUSR | S_IRGRP | S_IROTH))
		mode |= (S_IRUSR | S_IRGRP | S_IROTH);
	if (mode & (S_IWUSR | S_IWGRP | S_IWOTH))
		mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
	if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
		mode |= (S_IXUSR | S_IXGRP | S_IXOTH);
	perms |= (mode & S_IRUSR) ? 4 : 0;
	perms |= (mode & S_IWUSR) ? 2 : 0;
	perms |= (mode & S_IXUSR) ? 1 : 0;
	if ((ino = dfs_create(info, fn, perms, &fe)) == INV_INODE) /* TODO 7: Fill in all the parameters */
		return -ENOSPC;

	file_inode = new_inode(parent_inode->i_sb);
	if (!file_inode)
	{
		dfs_remove(info, fn); // Nothing to do, even if it fails
		return -ENOMEM;
	}
	printk(KERN_INFO "dfs: Created new VFS inode for #%d, let's fill in\n", ino);
	file_inode->i_ino = ino;
	file_inode->i_size = fe.size;
	file_inode->i_mode = S_IFREG | mode;
	file_inode->i_atime_sec = file_inode->i_mtime_sec = file_inode->i_ctime_sec = fe.timestamp;
	file_inode->i_atime_nsec = file_inode->i_mtime_nsec = file_inode->i_ctime_nsec = 0;
	file_inode->i_mapping->a_ops = &dfs_aops; /* TODO 8: Assign the address ops */
	file_inode->i_fop = &dfs_fops; /* TODO 9: Assign the file ops */
	if (insert_inode_locked(file_inode) < 0)
	{
		make_bad_inode(file_inode);
		iput(file_inode);
		dfs_remove(info, fn); // Nothing to do, even if it fails
		return -EIO;
	}
	d_instantiate(dentry, file_inode);
	unlock_new_inode(file_inode);

	return 0;
}
static int dfs_inode_unlink(struct inode *parent_inode, struct dentry *dentry)
{
	char fn[DEMO_FS_FILENAME_LEN + 1];
	dfs_info_t *info = (dfs_info_t *)(parent_inode->i_sb->s_fs_info);
	int ino;
	struct inode *file_inode = dentry->d_inode;

	printk(KERN_INFO "dfs: dfs_inode_unlink\n");

	strncpy(fn, dentry->d_name.name, DEMO_FS_FILENAME_LEN);
	fn[DEMO_FS_FILENAME_LEN] = 0;
	if ((ino = dfs_remove(info, fn)) == INV_INODE) /* TODO 6: Fill in all the parameters */
		return -EINVAL;

	inode_dec_link_count(file_inode);
	return 0;
}
// TODO: Populate the inode operations
// lookup, create, unlink
static struct inode_operations dfs_iops =
{
	lookup: dfs_inode_lookup,
	create: dfs_inode_create,
};

/*
 * Super-Block Operations
 */
static void dfs_put_super(struct super_block *sb)
{
	dfs_info_t *info = (dfs_info_t *)(sb->s_fs_info/* TODO 1: Get the private data from the VFS super block */);

	printk(KERN_INFO "dfs: dfs_put_super\n");
	if (info)
	{
		shut_browsing(info);
		kfree(info);
		sb->s_fs_info = NULL;
	}
}
static int dfs_write_inode(struct inode *inode, struct writeback_control *wbc)
{
	dfs_info_t *info = (dfs_info_t *)(inode->i_sb->s_fs_info);
	int size, timestamp, perms;

	printk(KERN_INFO "dfs: dfs_write_inode (i_ino = %ld)\n", inode->i_ino);

	if (!(S_ISREG(inode->i_mode))) // Real DFS deals only with regular files
		return 0;

	size = i_size_read(inode);
	timestamp = inode->i_mtime_sec > inode->i_ctime_sec ? inode->i_mtime_sec : inode->i_ctime_sec;
	perms = 0;
	perms |= (inode->i_mode & (S_IRUSR | S_IRGRP | S_IROTH)) ? 04 /* TODO 15: DFS permission for read */ : 0;
	perms |= (inode->i_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) ? 02 /* TODO 16: DFS permission for write */ : 0;
	perms |= (inode->i_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? 01 /* TODO 17: DFS permission for execute */ : 0;

	printk(KERN_INFO "dfs: dfs_write_inode with %d bytes @ %d secs w/ %o\n",
		size, timestamp, perms);

	return dfs_update(info, inode->i_ino, &size, &timestamp, &perms);
	//return dfs_update_file_entry(in; //TODO 18: Update the dfs entry */
}
//TODO: Populate super operations: put_super & write_inode 
static struct super_operations dfs_sops =
{
	put_super: dfs_put_super,
	write_inode: dfs_write_inode,
};

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
	printk(KERN_INFO "dfs: /dev/%s block size = %d\n",
		sb->s_bdev->bd_disk->disk_name, block_size(sb->s_bdev));
	if (!(info = (dfs_info_t *)(kzalloc(sizeof(dfs_info_t), GFP_KERNEL))))
		return -ENOMEM;
	info->vfs_sb = sb;
	if (init_browsing(info) < 0)
	{
		kfree(info);
		return -EIO;
	}
	/* Updating the VFS super_block */
	sb->s_magic = info->sb.type;
	sb->s_blocksize = info->sb.block_size;
	sb->s_blocksize_bits = get_bit_pos(info->sb.block_size);
	sb->s_type = &dfs; // file_system_type
	sb->s_op = &dfs_sops; // super block operations
	/*
	iget_locked:
	Search for the inode specified by ino in the inode cache and if 
	present return it with an increased reference count. This is for 
	file systems where the inode number is sufficient for unique 
	identification of an inode
	If the inode is not in cache, allocate a new inode and return 
	it locked, hashed, and with the I_NEW flag set. The file system 
	gets to fill it in before unlocking it via unlock_new_inode().
	*/

	dfs_root_inode = iget_locked(sb, ROOT_INODE_NUM); // obtain an inode from VFS
	if (!dfs_root_inode)
	{
		shut_browsing(info);
		kfree(info);
		return -EACCES;
	}
	if (dfs_root_inode->i_state & I_NEW) // allocated fresh now
	{
		printk(KERN_INFO "dfs: Got root's new VFS inode, let's fill in\n");
		dfs_root_inode->i_op = &dfs_iops; //TODO: inode operations
		dfs_root_inode->i_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		dfs_root_inode->i_fop = &dfs_dops; // TODO: file operations for directory
		dfs_root_inode->i_mapping->a_ops = &dfs_aops; //Address space operations// address operations
		unlock_new_inode(dfs_root_inode);
	}
	else
	{
		printk(KERN_INFO "dfs: Got root's VFS inode from inode cache\n");
	}
	/*
	d_make_root: allocates the root dentry. It is generally used 
	in the function that is called to read the superblock (fill_super), 
	which must initialize the root directory. So the root inode is obtained 
	from the superblock and is used as an argument to this function, to fill 
	the s_root field from the struct super_block structure.
	*/
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
	printk(KERN_INFO "dfs: dfs_mount: devname = %s\n", devname);

	 /* dfs_fill_super this will be called to fill the super block */
	return mount_bdev(fs_type, flags, devname, data, &dfs_fill_super);
}
static struct file_system_type dfs =
{
	name: "real_dfs", /* Name of our file system */
	fs_flags: FS_REQUIRES_DEV, /* Removes nodev from /proc/filesystems */
	mount:  dfs_mount,
	kill_sb: kill_block_super,
	owner: THIS_MODULE
};

static int __init dfs_init(void)
{
	int err;

	printk(KERN_INFO "dfs: dfs_init\n");
	err = register_filesystem(&dfs);
	return err;
}

static void __exit dfs_exit(void)
{
	printk(KERN_INFO "dfs: dfs_exit\n");
	unregister_filesystem(&dfs);
}

module_init(dfs_init);
module_exit(dfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("File System Module for real Simula File System");
