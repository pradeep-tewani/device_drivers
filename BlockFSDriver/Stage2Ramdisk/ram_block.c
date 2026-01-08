/* Disk on RAM Driver */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h> // For struct hd_geometry
#include <linux/blk-mq.h>
#include <linux/errno.h>

#include "ram_device.h"

#define RB_FIRST_MINOR 0
#define RB_MINOR_CNT 16

static u_int rb_major = 0;

/*
 * The internal structure representation of our Device
 */
static struct rb_device
{
	/* Size is the size of the device (in sectors) */
	sector_t size;
	/* Utility structure to store various parameters like queue depth, cmd size, ... */
	struct blk_mq_tag_set tag_set;
	/* Our request queue */
	struct request_queue *queue;
	/* This is kernel's representation of an individual disk device */
	struct gendisk *disk;
} rb_dev;

static int rb_open(struct gendisk *disk, fmode_t mode)
{
/* Need to figure out the way to extract the minor for versions higher that 
 * 6.10 as the field is removed and nothing is exposed outside the block 
 * layer to access the inode
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,10,0))
	unsigned unit = iminor(disk->part0->bd_inode);
#else
	unsigned unit = MINOR(disk->part0->bd_dev);
#endif

	printk(KERN_INFO "rb: Device is opened\n");
	printk(KERN_INFO "rb: Inode number is %d\n", unit);

	if (unit > RB_MINOR_CNT)
		return -ENODEV;
	return 0;
}

static void rb_close(struct gendisk *disk)
{
	printk(KERN_INFO "rb: Device is closed\n");
}

static int rb_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->heads = 1;
	geo->cylinders = 32;
	geo->sectors = 32;
	geo->start = 0;
	return 0;
}

/*
 * Actual Data transfer
 */
static int rb_transfer(struct request *req)
{
	//struct rb_device *dev = (struct rb_device *)(req->rq_disk->private_data);

	int dir = rq_data_dir(req);
	sector_t start_sector = blk_rq_pos(req);
	unsigned int sector_cnt = blk_rq_sectors(req);

#define BV_PAGE(bv) ((bv).bv_page)
#define BV_OFFSET(bv) ((bv).bv_offset)
#define BV_LEN(bv) ((bv).bv_len)
	struct bio_vec bv;
	struct req_iterator iter;

	sector_t sector_offset;
	unsigned int sectors;
	u8 *buffer;

	int ret = 0;

	//printk(KERN_DEBUG "rb: Dir:%d; Sec:%lld; Cnt:%d\n", dir, (unsigned long long)(start_sector), sector_cnt);

	sector_offset = 0;
	rq_for_each_segment(bv, req, iter)
	{
		/* 
		 * TODO 12: Initialize the buffer to point to data in bio_vec
         * Data may be available at particular offset in the page. 
         * The bv_page field holds the pointer to physical page
         * Use appropriate MACROS provided above to access 
         * the page and offest from bv
		 * Use page_address to get the virtual address for the page
         */
		if (BV_LEN(bv) % RB_SECTOR_SIZE != 0)
		{
			printk(KERN_ERR "rb: Should never happen: "
				"bio size (%d) is not a multiple of RB_SECTOR_SIZE (%d).\n"
				"This may lead to data truncation.\n",
				BV_LEN(bv), RB_SECTOR_SIZE);
			ret = -EIO;
		}
		/* 
         * TODO 13: Get the total number of sectors and assign to sectors variable 
		 * BV_LEN(bv) provides the data length in bytes. Convert this to sectors
         */
		printk(KERN_DEBUG "rb: Start Sector: %llu, Sector Offset: %llu; Buffer: %p; Length: %u sectors\n",
			(unsigned long long)(start_sector), (unsigned long long)(sector_offset), buffer, sectors);
		if (dir == WRITE) /* TODO 14: Write to the device */
		{
			//ramdevice_write(/* from sector */, buffer, /* number of sectors */);
		}
		else /* TODO 15: Read from the device */
		{
			//ramdevice_read(/* from sector */, buffer, /* number of sectors */);
		}
		sector_offset += sectors;
	}
	if (sector_offset != sector_cnt)
	{
		printk(KERN_ERR "rb: bio info doesn't match with the request info");
		ret = -EIO;
	}

	return ret;
}

/*
 * Represents a block I/O request for us to execute
 */
static blk_status_t rb_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	struct request *req;
	int status;

	/* Gets the current request */
	req = bd->rq;

	/* 
     * TODO 22: Start new request procedure w/ a timer to time the processing 
	 * use api void blk_mq_start_request(struct request *rq);
	 */

	status = ((rb_transfer(req) == 0) ? BLK_STS_OK : BLK_STS_IOERR);

	/* 
     * TODO 23: End request procedure use api 
	 * use void blk_mq_end_request(struct request *rq, blk_status_t error);
     */

	return BLK_STS_OK;
}

/*
 * These are the file operations that performed on the ram block device
 */
// TODO 8: Populate the file_operations .open, .release and .getgeo
static struct block_device_operations rb_fops =
{
	.owner = THIS_MODULE,
};

/*
 * This is the call back (request function) setup for the upper layers to process the requests in request queue(s)
 */
static struct blk_mq_ops rb_mq_ops =
{
	.queue_rq = rb_request
};

/*
 * This is the registration and initialization section of the ram block device
 * driver
 */
static int __init rb_init(void)
{
	int rv = 0;
	/*
	 * TODO 1: Set up our Disk On RAM (DOR)(ramdevice_init)
	 * and assign it to size field of struct rb_device
     */
	if (rb_dev.size < 0)
	{
		return (int)(rb_dev.size);
	}

	/* Get Registered */
	//TODO 2: Register the block device with register_blkdev(major, name)
	if (rb_major <= 0)
	{
		printk(KERN_ERR "rb: Unable to get Major Number\n");
		rv = -EBUSY;
		goto out_clean_ramdisk;
		//ramdevice_cleanup();
	}

	/*
	 * TODO 3: Allocate the tag-set using 
	 * int blk_mq_alloc_sq_tag_set(struct blk_mq_tag_set *set, 
			const struct blk_mq_ops *ops, unsigned int queue_depth,
        	unsigned int set_flags);
	   Pass queue_depth as 128 and flag as BLK_MQ_F_SHOULD_MERGE
	 */
		if (rv) {
		printk(KERN_ERR "rb: Failed to allocate tag set\n");
		goto out_unregister;
	}
	/*
	 * TODO 4: Allocate the gendisk structure using blk_mq_alloc_disk(set, lim, queuedata)
	 * Pass pointer to tag_set as first argument, queue_limits (lim) as NULL, 
	 * queuedata is driver private data-structure
	 */

	if (IS_ERR(rb_dev.disk)) {
		rv = PTR_ERR(rb_dev.disk);
		printk(KERN_ERR "rb: alloc_disk failure\n");
		goto out_free_tag_set;
	}
	/* Populate fields of newly allocated gendisk structure */
	/* TODO 5: Set the major field */
	/* TODO 6: Set the first_minor field */
	/* TODO 7: Set the fops field to the device operations */
	/* TODO 9: Set the private_data field to driver-specific internal data */
	/* 
     * TODO 10: Set the 'queue' field of rb_dev. Request Queue is allocated 
     * as a part of blk_mq_alloc_disk and the corresponding field 
     * in disk structure is 'queue'
     */

	rb_dev.disk->minors = RB_MINOR_CNT;
	/*
	 * You do not want partition information to show up in
	 * cat /proc/partitions set this flags
	 */
	//rb_dev.disk->flags = GENHD_FL_SUPPRESS_PARTITION_INFO;
	sprintf(rb_dev.disk->disk_name, "rb");
	/* Setting the capacity of the device in its gendisk structure */
	set_capacity(rb_dev.disk, rb_dev.size);

	/* TODO 11: Add the disk to the system with add_disk */
	if (rv != 0) {
		printk(KERN_ERR "Disk addition failed\n");
		goto out_cleanup_disk;
	}
	/* Now the disk is "live" */
	printk(KERN_INFO "rb: Ram Block driver initialised (%llu sectors; %llu bytes)\n",
		(unsigned long long)(rb_dev.size), (unsigned long long)(rb_dev.size * RB_SECTOR_SIZE));
	return 0;

out_cleanup_disk:
    put_disk(rb_dev.disk);
out_free_tag_set:
    blk_mq_free_tag_set(&rb_dev.tag_set);
out_unregister:
	unregister_blkdev(rb_major, "rb");
out_clean_ramdisk:
	ramdevice_cleanup();

    return rv;
}
/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit rb_cleanup(void)
{
	// TODO 17: Delete the gendisk with del_gendisk
	// TODO 18: Deallocate the gendisk with put_disk
	// TODO 19: Deallocate the tag_set with blk_mq_free_tag_set
	// TODO 20: Finally unregister the block device with unregister_blkdev
	// TODO 21: Clean up the RAM Device
}

module_init(rb_init);
module_exit(rb_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ram Block Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(rb_major);
