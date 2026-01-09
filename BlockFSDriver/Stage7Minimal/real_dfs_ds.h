#ifndef REAL_DFS_DS_H
#define REAL_DFS_DS_H

#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/spinlock.h>
#endif

#define DEMO_FS_TYPE 0x12345678 /* TODO 1: Magic Number for our file system */
#define DEMO_FS_BLOCK_SIZE 512 /* TODO 2: Block Size in bytes */
#define DEMO_FS_BLOCK_SIZE_BITS 9 /* TODO 3: log(DEMO_FS_BLOCK_SIZE) w/ base 2 */
#define DEMO_FS_ENTRY_SIZE 64 /* Entry size in bytes */
#define DEMO_FS_FILENAME_LEN 15
#define DEMO_FS_DATA_BLOCK_CNT ((DEMO_FS_ENTRY_SIZE - (DEMO_FS_FILENAME_LEN + 1) - (3 * 4)) / 4) /* TODO 4: Data Block Count */
typedef unsigned char byte1_t;
typedef unsigned int byte4_t;
typedef unsigned long long byte8_t;

typedef struct dfs_super_block
{
	/* NB Use the names in double quotes ("") as the field names.*/
	byte4_t type;/* TODO 5: Magic number "type" to identify the file system */
	byte4_t block_size;/* TODO 6: Unit of allocation "block_size" */
	byte4_t partition_size;/* TODO 7: Partition Size "partition_size" in blocks */
	byte4_t entry_size;/* TODO 8: Entry size "entry_size" in bytes */
	byte4_t entry_table_size;/* TODO 9: Entry table size "entry_table_size" in blocks */
	byte4_t entry_table_block_start;/* TODO 10: Entry table block start "entry_table_block_start" in blocks */
	byte4_t entry_count;/* TODO 11: Total entries "entry_count" in the file system */
	byte4_t data_block_start;/* TODO 12: Data block start "data_block_start in blocks */
	byte4_t reserved[DEMO_FS_BLOCK_SIZE / 4 - 8]; /* Making it of DEMO_FS_BLOCK_SIZE */
} dfs_super_block_t;

typedef struct dfs_file_entry
{
	/*
	 * TODO13 : Fill in the fields for the file entry for it to be of size DEMO_FS_ENTRY_SIZE.
	 * NB Use the names in double quotes ("") as the field names.
	 */
	char name[DEMO_FS_FILENAME_LEN + 1];/* File "name" of size DEMO_FS_FILENAME_LEN + 1 */
	byte4_t size;/* 4-byte file "size" in bytes */
	byte4_t timestamp;/* 4-byte file modify "timestamp" in seconds since Epoch */
	byte4_t perms;/* 4-byte file permissions "perms" only for user; Replicated for group & others */
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
