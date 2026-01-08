#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h> /* For errno */
#include <string.h> /* For strerror() */
#include <sys/ioctl.h> /* For ioctl() */
#include <linux/fs.h> /* For BLKGETSIZE64 */

#include "real_dfs_ds.h"

#define DFS_ENTRY_RATIO 0.10 /* 10% of all blocks */
#define DFS_ENTRY_TABLE_BLOCK_START 1

// TODO 1: Populate the super block, .type, .block_size, .entry_size
// and entry_table_block_start
dfs_super_block_t sb =
{
};
dfs_file_entry_t fe; /* All 0's */

void write_super_block(int dfs_handle, dfs_super_block_t *sb)
{
	// TODO 8: Update super block
}
void clear_file_entries(int dfs_handle, dfs_super_block_t *sb)
{
	int i;
	byte1_t block[DEMO_FS_BLOCK_SIZE];

	for (i = 0; i < sb->block_size / sb->entry_size; i++)
	{
		memcpy(block + i * sb->entry_size, &fe, sizeof(fe));
	}
	for (i = 0; i < sb->entry_table_size; i++)
	{
		write(dfs_handle, block, sizeof(block));
	}
}

int main(int argc, char *argv[])
{
	int dfs_handle;
	byte8_t size;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <partition's device file>\n", argv[0]);
		return 1;
	}
	dfs_handle = open(argv[1], O_RDWR);
	if (dfs_handle == -1)
	{
		fprintf(stderr, "Error formatting %s: %s\n", argv[1], strerror(errno));
		return 2;
	}
	// TODO 2: Use 'BLKGETSIZE64' ioctl to get the partition size.
	// The below call will provide size in bytes
	// ioctl(fd, BLKGETSIZE64, &size)
	if (-1)
	{
		fprintf(stderr, "Error getting size of %s: %s\n", argv[1], strerror(errno));
		return 3;
	}
	/* Initialize super block fields */
	/* TODO 3: Fill up the partition size in blocks */
	/* TODO 4: Fill up the entry table size in blocks */
	/* TODO 5: Fill up the total number of entries */

	sb.data_block_start = DFS_ENTRY_TABLE_BLOCK_START + sb.entry_table_size;

	printf("Partitioning %Ld byte sized %s ... ", size, argv[1]);
	fflush(stdout);
	// TODO 6: Update the super block by using write_super_block
	// TODO 7: Clear file enteries by using clear_file_entries

	close(dfs_handle);
	printf("done\n");

	return 0;
}
