#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "real_dfs_ds.h"

dfs_super_block_t sb;
byte1_t block[DEMO_FS_BLOCK_SIZE];
byte1_t *used_blocks;

int init_browsing(int dfs_handle)
{
	int i, j;
	dfs_file_entry_t fe;

	/* TODO 1: Read the superblock */
	if (0) 
	{
		printf("Unable to read the Super block\n");
		return -1;
	}
	/* TODO 2: Check for validity of Demo File System */
	if (0)
	{
		fprintf(stderr, "Invalid DFS detected. Giving up.\n");
		return -1;
	}
	/* TODO 11: Allocate an array of used_blocks */
	/* Mark used blocks */
	if (!used_blocks)
	{
		printf("Out of memory\n");
		return -2;
	}
	/* TODO 12: Mark the blocks till data block as used */

	/* TODO 13: Seek to the start of the entry table */
	for (i = 0; i < sb.entry_count; i++)
	{
		/* TODO 14: Read the fe */
		if (0)
		{
			printf("Failed to read the File Entry %d\n", i);
			return -1;
		}

		if (!fe.name[0]) continue;
		for (j = 0; j < DEMO_FS_DATA_BLOCK_CNT; j++)
		{
			if (fe.blocks[j] == 0) break;
			/* TODO 15: Mark the data blocks occupied by the file entry fe */
		}
	}

	printf("Welcome to DFS Browsing Shell v3.0\n\n");
	printf("Block size     : %d bytes\n", sb.block_size);
	printf("Partition size : %d blocks\n", sb.partition_size);
	printf("File entry size: %d bytes\n", sb.entry_size);
	printf("Entry tbl size : %d blocks\n", sb.entry_table_size);
	printf("Entry count    : %d\n", sb.entry_count);
	printf("\n");
	return 0;
}
void shut_browsing(int dfs_handle)
{
	/* TODO 16: Free up the used blocks array */ 
}
static int get_data_block(int dfs_handle)
{
	int i;

	for (i = sb.data_block_start; i < sb.partition_size; i++)
	{
		/* TODO 17: Get the free used block & mark it for use and return the same*/
	}
	return -1;
}
static void put_data_block(int dfs_handle, int i)
{
	/* TODO 18: Mark the block as unused */ 
}

void dfs_list(int dfs_handle)
{
	int i;
	dfs_file_entry_t fe;
	time_t ts;

	/* TODO 3A: Seek to the start of file entries table, to check for its existence */
	//lseek(dfs_handle, 0, SEEK_SET);
	for (i = 0; i < sb.entry_count; i++)
	{
		/* TODO 4A: Read the enteries into fe */
		if (0)
		{
			printf("Failed to read the File Entry %d\n", i);
			return;
		}
		if (!fe.name[0]) continue;
		ts = (time_t)(fe.timestamp);
		printf("%-15s  %10d bytes  %c%c%c  %s",
			fe.name, fe.size,
			fe.perms & 04 ? 'r' : '-',
			fe.perms & 02 ? 'w' : '-',
			fe.perms & 01 ? 'x' : '-',
			ctime(&ts)
			);
	}
}
void dfs_create(int dfs_handle, char *fn)
{
	int i;
	dfs_file_entry_t fe;

	/* TODO 3B: Seek to the start of file entries table, to check for its existence */
	for (i = 0; i < sb.entry_count; i++)
	{
		/* TODO 4B: Read the enteries into fe */
		if (0)
		{
			printf("Failed to read the File Entry %d\n", i);
			return;
		}
		if (!fe.name[0]) break;
		if (strcmp(fe.name, fn) == 0)
		{
			printf("File %s already exists\n", fn);
			return;
		}
	}
	if (i == sb.entry_count)
	{
		printf("No entries left\n");
		return;
	}

	/* TODO 5: Seek to the previous entry */

	/* TODO 6: Copy the filename */
	fe.name[DEMO_FS_FILENAME_LEN] = 0;
	fe.size = 0;
	fe.timestamp = 0; /* TODO 7: Fill in the current timestamp */
	fe.perms = 0; /* TODO 8: Fill in the permissions as "rwx" */
	for (i = 0; i < DEMO_FS_DATA_BLOCK_CNT; i++)
	{
		fe.blocks[i] = 0;
	}

	/* TODO 9: Update the entry into the entry table */
}


/* TODO 10: Populate the dfs_look up */
/* Returns fe index on success with fe pointing to desired entry and -1 on error */
/* 
 * Steps:
 * Seek to the start of the Entry table
 * Iterate over the enteries and check if the name matches with the one we are 
 * looking for and retun the same, otherwise return -1
 */
int dfs_lookup(int dfs_handle, char *fn, dfs_file_entry_t *fe)
{
	int i;
	return -1;
}
void dfs_remove(int dfs_handle, char *fn)
{
	int i, block_i;
	dfs_file_entry_t fe;

	if ((i = dfs_lookup(dfs_handle, fn, &fe)) == -1)
	{
		printf("File %s doesn't exist\n", fn);
		return;
	}
	/* Free up all allocated blocks, if any */
	for (block_i = 0; block_i < DEMO_FS_DATA_BLOCK_CNT; block_i++)
	{
		if (!fe.blocks[block_i])
		{
			break;
		}
		put_data_block(dfs_handle, 0/* TODO 19: Block to put back / free */);
	}
	memset(&fe, 0, sizeof(dfs_file_entry_t));

	/* TODO 20: Seek & update the file entry in filesystem */
}
void dfs_update(int dfs_handle, char *fn, int *size, int update_ts, int *perms)
{
	int i;
	dfs_file_entry_t fe;

	if ((i = dfs_lookup(dfs_handle, fn, &fe)) == -1)
	{
		printf("File %s doesn't exist\n", fn);
		return;
	}
	if (size) fe.size = 0; /* TODO 21: Update with the new file size */
	if (update_ts) fe.timestamp = 0; /* TODO 22: Update to current time */
	if (perms && (*perms <= 07)) fe.perms = 0; /* TODO 23: Update with the new file permissions */
	/* TODO 24: Seek to the current entry & update the same in the filesystem */
}
void dfs_chperm(int dfs_handle, char *fn, int perm)
{
	dfs_update(dfs_handle, fn, NULL, 0, &perm);
}
void dfs_read(int dfs_handle, char *fn)
{
	int i, block_i, already_read, rem_to_read, to_read;
	dfs_file_entry_t fe;

	if ((i = dfs_lookup(dfs_handle, fn, &fe)) == -1)
	{
		printf("File %s doesn't exist\n", fn);
		return;
	}
	/* TODO 25: Verify if the file has read permission */
	if (0)
	{
		printf("File %s doesn't have read permissions\n", fn);
		return;
	}
	already_read = 0;
	rem_to_read = fe.size;
	for (block_i = 0; block_i < DEMO_FS_DATA_BLOCK_CNT; block_i++)
	{
		if (!fe.blocks[block_i]) break;
		to_read = (rem_to_read >= sb.block_size) ? sb.block_size : rem_to_read;
		lseek(dfs_handle, fe.blocks[block_i] * sb.block_size, SEEK_SET);
		read(dfs_handle, block, to_read);
		write(1, block, to_read);
		already_read += to_read;
		rem_to_read -= to_read;
		if (!rem_to_read) break;
	}
}
void dfs_write(int dfs_handle, char *fn)
{
	int i, cur_read_i, to_read, cur_read, total_size, block_i;
	dfs_file_entry_t fe;

	if ((i = dfs_lookup(dfs_handle, fn, &fe)) == -1)
	{
		printf("File %s doesn't exist\n", fn);
		return;
	}
	/* TODO 26: Verify if the file has write permission */
	if (0)
	{
		printf("File %s doesn't have write permissions\n", fn);
		return;
	}
	/* Free up all previously allocated blocks, if any */
	for (block_i = 0; block_i < DEMO_FS_DATA_BLOCK_CNT; block_i++)
	{
		if (!fe.blocks[block_i])
		{
			break;
		}
		/* TODO 27 : Release the data block */
	}
	/* Let's get data & write */
	cur_read_i = 0;
	to_read = sb.block_size;
	total_size = 0;
	block_i = 0;
	while ((cur_read = read(0, block + cur_read_i, to_read)) > 0)
	{
		if (cur_read == to_read)
		{
			/* Write this block */
			if (block_i == DEMO_FS_DATA_BLOCK_CNT)
				break; /* File size limit */
			if ((fe.blocks[block_i] = get_data_block(dfs_handle)) == -1)
				break; /* File system full */
			lseek(dfs_handle, 0 /* TODO 28A: Seek to the corresponding data block */, SEEK_SET);
			write(dfs_handle, block, sb.block_size);
			block_i++;
			total_size += sb.block_size;
			/* Reset various variables */
			cur_read_i = 0;
			to_read = sb.block_size;
		}
		else
		{
			cur_read_i += cur_read;
			to_read -= cur_read;
		}
	}
	if ((cur_read <= 0) && (cur_read_i))
	{
		/* Write this partial block */
		if ((block_i != DEMO_FS_DATA_BLOCK_CNT) &&
			((fe.blocks[block_i] = get_data_block(dfs_handle)) != -1))
		{
			lseek(dfs_handle, 0, /* TODO 28B: Seek to the corresponding data block */ SEEK_SET);
			write(dfs_handle, block, cur_read_i);
			total_size += cur_read_i;
		}
	}

	fe.size = 0; /* TODO 29: Update the file size */
	fe.timestamp = time(NULL);
	lseek(dfs_handle, sb.entry_table_block_start * sb.block_size + i * sb.entry_size, SEEK_SET);
	write(dfs_handle, &fe, sizeof(dfs_file_entry_t));
}

void usage(void)
{
	printf("Supported commands:\n");
	printf("\t?\tquit\tlist\tcreate <file>\tremove <file>\n");
	printf("\t\tchperm <0-7> <file>\tread <file>\twrite <file>\n");
}

void browse_dfs(int dfs_handle)
{
	int done;
	char cmd[256], *fn;
	int ret, perm;

	if (init_browsing(dfs_handle) != 0)
	{
		return;
	}

	done = 0;
	while (!done)
	{
		printf(" $> ");
		ret = scanf("%[^\n]", cmd);
		if (ret < 0)
		{
			done = 1;
			printf("\n");
			continue;
		}
		else
		{
			getchar();
			if (ret == 0) continue;
		}
		if (strcmp(cmd, "?") == 0)
		{
			usage();
			continue;
		}
		else if (strcmp(cmd, "quit") == 0)
		{
			done = 1;
			continue;
		}
		else if (strcmp(cmd, "list") == 0)
		{
			dfs_list(dfs_handle);
			continue;
		}
		else if (strncmp(cmd, "create", 6) == 0)
		{
			if (cmd[6] == ' ')
			{
				fn = cmd + 7;
				while (*fn == ' ') fn++;
				if (*fn != '\0')
				{
					dfs_create(dfs_handle, fn);
					continue;
				}
			}
		}
		else if (strncmp(cmd, "remove", 6) == 0)
		{
			if (cmd[6] == ' ')
			{
				fn = cmd + 7;
				while (*fn == ' ') fn++;
				if (*fn != '\0')
				{
					dfs_remove(dfs_handle, fn);
					continue;
				}
			}
		}
		else if (strncmp(cmd, "chperm", 6) == 0)
		{
			if (cmd[6] == ' ')
			{
				perm = cmd[7] - '0';
				if ((0 <= perm) && (perm <= 7) &&  (cmd[8] == ' '))
				{
					fn = cmd + 9;
					while (*fn == ' ') fn++;
					if (*fn != '\0')
					{
						dfs_chperm(dfs_handle, fn, perm);
						continue;
					}
				}
			}
		}
		else if (strncmp(cmd, "read", 4) == 0)
		{
			if (cmd[4] == ' ')
			{
				fn = cmd + 5;
				while (*fn == ' ') fn++;
				if (*fn != '\0')
				{
					dfs_read(dfs_handle, fn);
					continue;
				}
			}
		}
		else if (strncmp(cmd, "write", 5) == 0)
		{
			if (cmd[5] == ' ')
			{
				fn = cmd + 6;
				while (*fn == ' ') fn++;
				if (*fn != '\0')
				{
					dfs_write(dfs_handle, fn);
					continue;
				}
			}
		}
		printf("Unknown/Incorrect command: %s\n", cmd);
		usage();
	}

	shut_browsing(dfs_handle);
}

int main(int argc, char *argv[])
{
	char *dfs_file;
	int dfs_handle;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <partition's device file>\n", argv[0]);
		return 1;
	}
	dfs_file = argv[1];
	dfs_handle = open(dfs_file, O_RDWR);
	if (dfs_handle == -1)
	{
		fprintf(stderr, "Unable to browse DFS over %s\n", dfs_file);
		return 2;
	}
	browse_dfs(dfs_handle);
	close(dfs_handle);
	return 0;
}
