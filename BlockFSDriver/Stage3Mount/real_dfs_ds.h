#ifndef REAL_DFS_DS_H
#define REAL_DFS_DS_H

#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/spinlock.h>
#endif
/* Refer /usr/include/linux/magic.h for magic numbers already in use */
#define DEMO_FS_TYPE /* TODO 1: Magic Number for our file system. Use any 4 byte number*/
#define DEMO_FS_BLOCK_SIZE /* TODO 2: Block Size in bytes */
#define DEMO_FS_BLOCK_SIZE_BITS /* TODO 3: log(DEMO_FS_BLOCK_SIZE) w/ base 2 */
#define DEMO_FS_ENTRY_SIZE 64 /* Entry size in bytes */
#define DEMO_FS_FILENAME_LEN 15
/*
 * TODO 4: Calculate Data Block Count per File
 * This is restricted by File Entry Size (64 bytes). Refer dfs_file_entry below
 * We need 4 bytes to hold the index of each block and few bytes in dfs_file_entry
 * has been used to accomodate fields like file name, size, permissions, Timestamp
 * Remaining needs to be kept for data block indices
 */
#define DEMO_FS_DATA_BLOCK_CNT /* TODO 4: Data Block Count */
typedef unsigned char byte1_t;
typedef unsigned int byte4_t;
typedef unsigned long long byte8_t;

typedef struct dfs_super_block
{
	/* All of data type byte4_t */
	/* NB Use the names in double quotes ("") as the field names.*/
	/* TODO 5: Magic number "type" to identify the file system */
	/* TODO 6: Unit of allocation "block_size" */
	/* TODO 7: Partition Size "partition_size" in blocks */
	/* TODO 8: Entry size "entry_size" in bytes */
	/* TODO 9: Entry table size "entry_table_size" in blocks */
	/* TODO 10: Entry table block start "entry_table_block_start" in blocks */
	/* TODO 11: Total entries "entry_count" in the file system */
	/* TODO 12: Data block start "data_block_start in blocks */
	byte4_t reserved[DEMO_FS_BLOCK_SIZE / 4 - 8]; /* Making it of DEMO_FS_BLOCK_SIZE */
} dfs_super_block_t;

typedef struct dfs_file_entry
{
	/*
	 * TODO 13 : Fill in the fields for the file entry for it to be of size DEMO_FS_ENTRY_SIZE.
	 * NB Use the names in double quotes ("") as the field names.
	 */
	/* File "name" of size DEMO_FS_FILENAME_LEN + 1 */
	/* 4-byte file "size" in bytes */
	/* 4-byte file modify "timestamp" in seconds since Epoch */
	/* 4-byte file permissions "perms" only for user; Replicated for group & others */
	/* Array of 4-byte indices of file's data "blocks" */
	byte4_t blocks[DEMO_FS_DATA_BLOCK_CNT];
} dfs_file_entry_t;

#ifdef __KERNEL__
typedef struct dfs_info
{
	struct super_block *vfs_sb; /* Super block structure from VFS for this fs */
	dfs_super_block_t sb; /* Our fs super block */
	byte1_t *used_blocks; /* Used blocks tracker */
	spinlock_t lock; /* Used for protecting used_blocks access */
} dfs_info_t;
#endif

#endif
