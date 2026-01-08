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

int init_browsing(int dfs_handle)
{
	/* TODO 1: Read the superblock */
	if (0) {
		printf("Unable to read the Super block\n");
		return -1;
	}
	/* TODO 2: Check for validity of Demo File System */
	if (0)
	{
		fprintf(stderr, "Invalid DFS detected. Giving up.\n");
		return -1;
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
}

void dfs_list(int dfs_handle)
{
	int i;
	dfs_file_entry_t fe;
	time_t ts;

	/* TODO 3A: Seek to the start of file entries table, to check for its existence */
	
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
	fe.timestamp = 0; /* TODO 7: Fill in the current timestamp, use time API*/
	fe.perms = 0; /* TODO 8: Fill in the permissions as "rwx" */
	for (i = 0; i < DEMO_FS_DATA_BLOCK_CNT; i++)
	{
		fe.blocks[i] = 0;
	}

	/* TODO 9: Update the entry into the entry table */
}
void dfs_remove(int dfs_handle, char *fn)
{
	printf("%s not implemented\n", __func__);
}
void dfs_update(int dfs_handle, char *fn, int *size, int update_ts, int *perms)
{
	printf("%s not implemented\n", __func__);
}
void dfs_chperm(int dfs_handle, char *fn, int perm)
{
	dfs_update(dfs_handle, fn, NULL, 0, &perm);
}
void dfs_read(int dfs_handle, char *fn)
{
	printf("%s not implemented\n", __func__);
}
void dfs_write(int dfs_handle, char *fn)
{
	printf("%s not implemented\n", __func__);
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
