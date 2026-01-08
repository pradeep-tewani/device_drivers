#include <linux/string.h>

#include "partition.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define SECTOR_SIZE 512
#define MBR_SIZE SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 // sizeof(PartEntry)
#define PARTITION_TABLE_SIZE 64 // sizeof(PartTable)
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55
#define DISK_SIGNATURE 0x36E5756D

typedef struct
{
	unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
	unsigned char start_head;
	unsigned char start_sec:6;
	unsigned char start_cyl_hi:2;
	unsigned char start_cyl;
	unsigned char part_type;
	unsigned char end_head;
	unsigned char end_sec:6;
	unsigned char end_cyl_hi:2;
	unsigned char end_cyl;
	unsigned int abs_start_sec;
	unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

/*
Device Boot StartCHS    EndCHS        StartLBA     EndLBA    Sectors  Size Id Type
/dev/rb1    0,0,2       9,0,32               1        319        319  159K 83 Linux
/dev/rb2    10,0,1      19,0,32            320        639        320  160K  5 Extended
/dev/rb3    20,0,1      31,0,32            640       1023        384  192K 83 Linux
*/

// TODO 1: Create a Partitition table as per above table
// Use appropriate values for start_head, end_head, start_cyl, end_cyl
// start_sec, end_sec, abs_start_sec and sec_in_part
static PartTable def_part_table =
{
	{
		boot_type: 0x00,
		part_type: 0x83,
	},
	{
		boot_type: 0x00,
		part_type: 0x05,
	},
	{
		boot_type: 0x00,
		part_type: 0x83,
	},
	{
	}
};
/*
/dev/rb5    10,0,2      13,0,32            321        447        127 65024 83 Linux
/dev/rb6    14,0,2      17,0,32            449        575        127 65024 83 Linux
/dev/rb7    18,0,2      19,0,32            577        639         63 32256 83 Linux
*/
//TODO 2: Create the Extended partition table as per the above table
static unsigned int def_log_part_br_cyl[] = {0x0A, 0x0E, 0x12};
static const PartTable def_log_part_table[] =
{
	{
		{
			boot_type: 0x00,
			part_type: 0x83,
		},
		{
			boot_type: 0x00,
			part_type: 0x05,
		},
	},
	{
		{
			boot_type: 0x00,
			part_type: 0x83,
		},
		{
			boot_type: 0x00,
			part_type: 0x05,
		},
	},
	{
		{
			boot_type: 0x00,
			part_type: 0x83,
		},
	}
};
/*
MBR
--------------------------------------------
| Address | Description     | Size (Bytes) |
-------------------------------------------
|0          Bootstrap Code  | 440          |
|440        Disk Signature  | 4
|444        Usually Null    | 2            |
|446        Partition Table | 64           |
            (Four 16 Bytes  |
            Entries)        |
|510        MBR Signature   | 2            |
--------------------------------------------
*/
static void copy_mbr(u8 *disk)
{
	memset(disk, 0x0, MBR_SIZE);
	// TODO 3: Copy the disk signature
	// TODO 4: Copy the Partition table
	// TODO 5: Copy the MBR signature
}
static void copy_br(u8 *disk, int start_cylinder, const PartTable *part_table)
{
	disk += (start_cylinder * 32 /* sectors / cyl */ * SECTOR_SIZE);
	memset(disk, 0x0, BR_SIZE);
	// TODO 6: Copy the partition table
	// TODO 7: Copy the BR Signature
}
void copy_mbr_n_br(u8 *disk)
{
	int i;

	copy_mbr(disk);
	for (i = 0; i < ARRAY_SIZE(def_log_part_table); i++)
	{
		copy_br(disk, def_log_part_br_cyl[i], &def_log_part_table[i]);
	}
}
