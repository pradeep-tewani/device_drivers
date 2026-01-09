#include <linux/version.h> /* For LINUX_VERSION_CODE */
#include <linux/fs.h> /* For struct super_block */
#include <linux/errno.h> /* For error codes */
#include <linux/buffer_head.h> /* struct buffer_head, sb_bread, ... */
#include <linux/blkdev.h> /* block_size, ... */
#include <linux/string.h> /* For memcpy */
#include <linux/vmalloc.h> /* For vmalloc, ... */
#include <linux/time.h> /* For get_seconds, ... */

#include "real_dfs_ds.h"
#include "real_dfs_ops.h"

static int read_sb_from_real_dfs(dfs_info_t *info, dfs_super_block_t *sb)
{
	struct buffer_head *bh;
	int sb_block_start = 0; /* TODO 2: Initialize to super block's block number */

	if (!(bh = sb_bread(info->vfs_sb, sb_block_start)))
	{
		return -EIO;
	}
	memcpy(sb, bh->b_data, sizeof(dfs_super_block_t));
	brelse(bh);
	return 0;
}
static int read_from_real_dfs(dfs_info_t *info, byte4_t block, byte4_t offset, void *buf, byte4_t len)
{
	byte4_t fs_block_size = info->sb.block_size;
	byte4_t bd_block_size = block_size(info->vfs_sb->s_bdev);
	byte4_t abs;
	struct buffer_head *bh;

	// Translating the real SFS block numbering to underlying block device block numbering, for sb_bread()
	abs = block * fs_block_size + offset; /* TODO 10A: Compute the absolute total byte offset */
	block = abs / bd_block_size;
	offset = abs % bd_block_size;
	if (offset + len > bd_block_size) // Should never happen
	{
		return -EINVAL;
	}
	if (!(bh = sb_bread(info->vfs_sb, block)))
	{
		return -EIO;
	}
	memcpy(buf, bh->b_data + offset, len);
	brelse(bh);
	return 0;
}
static int write_to_real_dfs(dfs_info_t *info, byte4_t block, byte4_t offset, void *buf, byte4_t len)
{
	byte4_t fs_block_size = info->sb.block_size;
	byte4_t bd_block_size = block_size(info->vfs_sb->s_bdev);
	byte4_t abs;
	struct buffer_head *bh;

	// Translating the real SFS block numbering to underlying block device block numbering, for sb_bread()
	abs = block * fs_block_size + offset; /* TODO 10B: Compute the absolute total byte offset */
	block = abs / bd_block_size;
	offset = abs % bd_block_size;
	if (offset + len > bd_block_size) // Should never happen
	{
		return -EINVAL;
	}
	if (!(bh = sb_bread(info->vfs_sb, block)))
	{
		return -EIO;
	}
	memcpy(bh->b_data + offset, buf, len);
	mark_buffer_dirty(bh);
	brelse(bh);
	return 0;
}
static int read_entry_from_real_dfs(dfs_info_t *info, int ino, dfs_file_entry_t *fe)
{
	return read_from_real_dfs(info, info->sb.entry_table_block_start /* TODO 8A: FS Block Number */, ino * info->sb.entry_size   /* TODO 9A: FS Block Offset */, fe, sizeof(dfs_file_entry_t));
}
static int write_entry_to_real_dfs(dfs_info_t *info, int ino, dfs_file_entry_t *fe)
{
	return write_to_real_dfs(info, info->sb.entry_table_block_start/* TODO 8B: FS Block Number */,
				ino * info->sb.entry_size /* TODO 9B: FS Block Offset */, fe, sizeof(dfs_file_entry_t));
}
int dfs_get_file_entry(dfs_info_t *info, int vfs_ino, dfs_file_entry_t *fe)
{
	return read_entry_from_real_dfs(info, V2S_INODE_NUM(vfs_ino)/* TODO 11A: Pass our file entry number */, fe);
}
int dfs_update_file_entry(dfs_info_t *info, int vfs_ino, dfs_file_entry_t *fe)
{
	return write_entry_to_real_dfs(info, V2S_INODE_NUM(vfs_ino)/* TODO 11B: Pass our file entry number */, fe);
}

int init_browsing(dfs_info_t *info)
{
	byte1_t *used_blocks;
	int i, j;
	dfs_file_entry_t fe;
	int retval;
	/* TODO 1: Read super block from block device */
	if ((retval = read_sb_from_real_dfs(info, &info->sb)) < 0)
	{
		return retval;
	}
	if (info->sb.type != DEMO_FS_TYPE /* TODO 3: Check for validity of Demo File System */)
	{
		printk(KERN_ERR "Invalid DFS detected. Giving up.\n");
		return -EINVAL;
	}

	/* TODO 4: Allocate the used blocks */
	used_blocks = (byte1_t *)vmalloc(info->sb.partition_size);
	if (!used_blocks)
	{
		return -ENOMEM;
	}
	/* TODO 5: Mark the blocks as used till data block start*/
	for (i = 0; i < info->sb.data_block_start; i++)
	{
		used_blocks[i] = 1;
			
	}
	/* TODO 6: Mark remaining blocks as unused */
	for (; i < info->sb.partition_size; i++)
	{
			used_blocks[i] = 0;
	}

	for (i = 0; i < info->sb.entry_count; i++)
	{
		/* TODO 7: Read the file entry from the block device */
		if ((retval = read_entry_from_real_dfs(info, i, &fe)) < 0)
		{
			vfree(used_blocks);
			return retval;
		}
		if (!fe.name[0]) continue;
		for (j = 0; j < DEMO_FS_DATA_BLOCK_CNT; j++)
		{
			if (fe.blocks[j] == 0) break;
			/* TODO 11: Mark the used blocks */ 
			used_blocks[fe.blocks[j]] = 1;
		}
	}

	info->used_blocks = used_blocks;
	info->vfs_sb->s_fs_info = info;
	spin_lock_init(&info->lock);
	return 0;
}
void shut_browsing(dfs_info_t *info)
{
	if (info->used_blocks)
		vfree(info->used_blocks);
}

int dfs_get_data_block(dfs_info_t *info)
{
	int i;

	spin_lock(&info->lock); // To prevent racing on used_blocks access
	for (i = info->sb.data_block_start; i < info->sb.partition_size; i++)
	{
		//TODO: Mark the free block and return the index
		if (!info->used_blocks[i])
		{
			info->used_blocks[i] = 1;
			spin_unlock(&info->lock);
			return i;
		}
	}
	spin_unlock(&info->lock);
	return INV_BLOCK;
}
void dfs_put_data_block(dfs_info_t *info, int i)
{
	spin_lock(&info->lock); // To prevent racing on used_blocks access
	//TODO: Mark the block as unused
	info->used_blocks[i] = 0;
	spin_unlock(&info->lock);
}
int dfs_list(dfs_info_t *info, struct file *file, struct dir_context *ctx)
{
	loff_t pos;
	int ino;
	dfs_file_entry_t fe;
	int retval;

	pos = 1; /* Starts at 1 as . is position 0 & .. is position 1 */
	for (ino = 0; ino < info->sb.entry_count; ino++)
	{
		if ((retval = read_entry_from_real_dfs(info, ino/* TODO 15: Get the ino'th file entry into fe */, &fe)) < 0)
			return retval;
		if (!fe.name[0]) continue;
		pos++; /* Position of this file */
		if (ctx->pos == pos)
		{
			if (!dir_emit(ctx, fe.name, strlen(fe.name), S2V_INODE_NUM(ino), DT_REG))
			{
				return -ENOSPC;
			}
			ctx->pos++;
		}
	}
	return 0;
}
int dfs_create(dfs_info_t *info, char *fn, int perms, dfs_file_entry_t *fe)
/* This function is called only if the file doesn't exist */
{
	int ino, free_ino, i;

	free_ino = INV_INODE;
	for (ino = 0; ino < info->sb.entry_count; ino++)
	{
		//TODO: Assign the available inode
		read_entry_from_real_dfs(info, ino, fe);
		if (!fe->name[0]) {
			free_ino = ino;
			break;
		}
	}
	if (free_ino == INV_INODE)
	{
		printk(KERN_ERR "No entries left\n");
		return INV_INODE;
	}

	strncpy(fe->name, fn, DEMO_FS_FILENAME_LEN);
	fe->name[DEMO_FS_FILENAME_LEN] = 0;
	fe->size = 0;
	fe->timestamp = ktime_get_real_seconds(); /* TODO 16: timestamp */
	fe->perms = perms; /* TODO 17: Permissions passed */
	for (i = 0; i < DEMO_FS_DATA_BLOCK_CNT; i++)
	{
		fe->blocks[i] = 0;
	}

	//TODO: Write back the Entry
	if (write_entry_to_real_dfs(info, ino, fe) < 0)
		return INV_INODE;

	return S2V_INODE_NUM(free_ino);
}
int dfs_lookup(dfs_info_t *info, char *fn, dfs_file_entry_t *fe)
{
	int ino;

	for (ino = 0; ino < info->sb.entry_count; ino++)
	{
		//TODO: Read entry from the filesystem
		if (read_entry_from_real_dfs(info, ino, fe) < 0)
			return INV_INODE;
		if (!fe->name[0]) continue;
		if (strcmp(fe->name, fn) == 0) return S2V_INODE_NUM(ino); /* TODO 18: Return the VFS Inode Number */
	}

	return INV_INODE;
}
int dfs_remove(dfs_info_t *info, char *fn)
{
	int vfs_ino, block_i;
	dfs_file_entry_t fe;

	//TODO: Look up for the entry
	vfs_ino = dfs_lookup(info, fn, &fe);
	if (vfs_ino == INV_INODE)
	{
		printk(KERN_ERR "File %s doesn't exist\n", fn);
		return INV_INODE;
	}
	/* Free up all allocated blocks, if any */
	for (block_i = 0; block_i < DEMO_FS_DATA_BLOCK_CNT; block_i++)
	{
		if (!fe.blocks[block_i])
		{
			break;
		}
		//TODO: Release the block
		dfs_put_data_block(info, fe.blocks[block_i]);
	}
	/* TODO 19: Initialize fe entry to zero's */
	memset(&fe, 0, sizeof(dfs_file_entry_t));

	//TODO: Update the entry to filesystem
	if (dfs_update_file_entry(info, vfs_ino, &fe) < 0)
		return INV_INODE;

	return vfs_ino;
}
int dfs_update(dfs_info_t *info, int vfs_ino, int *size, int *timestamp, int *perms)
{
	dfs_file_entry_t fe;
	int i;
	int retval;

	//TODO: Get the file entry
	if ((retval = dfs_get_file_entry(info, vfs_ino, &fe)) < 0)
	{
		return retval;
	}
	if (size) fe.size = *size;
	if (timestamp) fe.timestamp = *timestamp;
	if (perms && (*perms <= 07)) fe.perms = *perms;

	for (i = (fe.size + info->sb.block_size - 1) / info->sb.block_size; i < DEMO_FS_DATA_BLOCK_CNT; i++)
	{
		if (fe.blocks[i])
		{
			dfs_put_data_block(info, fe.blocks[i]);
			fe.blocks[i] = 0;
		}
	}

	//TODO: Update the file entry
	return dfs_update_file_entry(info, vfs_ino, &fe);
}
