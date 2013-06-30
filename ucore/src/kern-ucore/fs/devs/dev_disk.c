#include <types.h>
#include <mmu.h>
#include <slab.h>
#include <sem.h>
#include <ide.h>
#include <inode.h>
#include <dev.h>
#include <ide.h>
#include <vfs.h>
#include <iobuf.h>
#include <error.h>
#include <assert.h>

/*
#define DISK0_BLKSIZE                   PGSIZE
#define DISK0_BUFSIZE                   (4 * DISK0_BLKSIZE)
#define DISK0_BLK_NSECT                 (DISK0_BLKSIZE / SECTSIZE)
static char *disk0_buffer;
static semaphore_t disk0_sem;
*/


#define MAX_IDE 						4
#define MBR_MAGIC 						0xAA55
#define MBR_PTB_OFFSET					446
#define MBR_PTB_ENTRY_LEN				16
#define DISK_BLKSIZE 					PGSIZE
#define DISK_BUFSIZE 					(4 * DISK_BLKSIZE)
#define DISK_BLK_NSECT 					(DISK_BLKSIZE / SECTSIZE)

static char *disk_buffer[MAX_IDE];
static semaphore_t disk_sem[MAX_IDE];

struct device* no2dev[128];

struct partitiondesc {
	unsigned int valid:8;
	unsigned int start_head:8;
	unsigned int start_sector:6;
	unsigned int start_cylinder:10;
	unsigned int type:8;
	unsigned int end_head:8;
	unsigned int end_sector:6;
	unsigned int end_cylinder:10;
	unsigned int offset_sectors:32;
	unsigned int total_sectors:32;
};

static void lock_disk(unsigned short ideno)
{
	down(&(disk_sem[ideno]));
}

static void unlock_disk(unsigned short ideno)
{
	up(&(disk_sem[ideno]));
}

static int disk_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int disk_close(struct device *dev)
{
	return 0;
}

static void disk_read_blks_nolock(unsigned short ideno, uint32_t blkno, uint32_t nblks)
{
	int ret;
	uint32_t sectno = blkno * DISK_BLK_NSECT, nsecs =
	    nblks * DISK_BLK_NSECT;
	if ((ret =
	     ide_read_secs(ideno, sectno, disk_buffer[ideno], nsecs)) != 0) {
		panic
		    ("disk%d: read blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     ideno, blkno, sectno, nblks, nsecs, ret);
	}
}

static void disk_write_blks_nolock(unsigned short ideno, uint32_t blkno, uint32_t nblks)
{
	int ret;
	uint32_t sectno = blkno * DISK_BLK_NSECT, nsecs =
	    nblks * DISK_BLK_NSECT;
	if ((ret =
	     ide_write_secs(ideno, sectno, disk_buffer[ideno], nsecs)) != 0) {
		panic
		    ("disk%d: write blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     ideno, blkno, sectno, nblks, nsecs, ret);
	}
}

static int disk_io(struct device *dev, struct iobuf *iob, bool write)
{
	off_t offset = dev->d_offset_sectors * SECTSIZE + iob->io_offset;
	size_t resid = iob->io_resid;
	uint32_t blkno = offset / DISK_BLKSIZE;
	uint32_t nblks = resid / DISK_BLKSIZE;

	/* don't allow I/O that isn't block-aligned */
	if ((offset % DISK_BLKSIZE) != 0 || (resid % DISK_BLKSIZE) != 0) {
		return -E_INVAL;
	}

	/* don't allow I/O past the end of disk0 */
	if (blkno + nblks > dev->d_blocks) {
		return -E_INVAL;
	}

	/* read/write nothing ? */
	if (nblks == 0) {
		return 0;
	}

	lock_disk(dev->d_ideno);
	while (resid != 0) {
		size_t copied, alen = DISK_BUFSIZE;
		if (write) {
			iobuf_move(iob, disk_buffer[dev->d_ideno], alen, 0, &copied);
			assert(copied != 0 && copied <= resid
			       && copied % DISK_BLKSIZE == 0);
			nblks = copied / DISK_BLKSIZE;
			disk_write_blks_nolock(dev->d_ideno, blkno, nblks);
		} else {
			if (alen > resid) {
				alen = resid;
			}
			nblks = alen / DISK_BLKSIZE;
			disk_read_blks_nolock(dev->d_ideno, blkno, nblks);
			iobuf_move(iob, disk_buffer[dev->d_ideno], alen, 1, &copied);
			assert(copied == alen && copied % DISK_BLKSIZE == 0);
		}
		resid -= copied, blkno += nblks;
	}
	unlock_disk(dev->d_ideno);
	return 0;
}

static int disk_ioctl(struct device *dev, int op, void *data)
{
	return -E_UNIMP;
}

static void disk_device_init(struct device *dev, struct partitiondesc *pd, 
									unsigned short ideno, unsigned short devno)
{
	memset(dev, 0, sizeof(*dev));
	//static_assert(DISK0_BLKSIZE % SECTSIZE == 0);
	if (!ide_device_valid(ideno)) {
		panic("ide %d device isn't available.\n", ideno);
	}
	dev->d_ideno = ideno;
	dev->d_devno = devno;
	dev->d_offset_sectors = pd->offset_sectors;
	dev->d_total_sectors = pd->total_sectors;

	//dev->d_blocks = ide_device_size(DISK0_DEV_NO) / DISK0_BLK_NSECT;
	dev->d_blocks = pd->total_sectors * SECTSIZE / 8;
	dev->d_blocksize = DISK_BLKSIZE;

	dev->d_open = disk_open;
	dev->d_close = disk_close;
	dev->d_io = disk_io;
	dev->d_ioctl = disk_ioctl;

	no2dev[devno] = dev;
	
	//static_assert(DISK0_BUFSIZE % DISK0_BLKSIZE == 0);
	//kprintf("disk0_device_init %d\n", dev->d_blocks);
}

void create_partition_dev(struct partitiondesc* pd, 
	unsigned short ideno, unsigned short devno, const char* name)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("%s: dev_create_node.\n", name);
	}
	disk_device_init(vop_info(node, device), pd, ideno, devno);
	int ret;
	if ((ret = vfs_add_dev(name, node, 1)) != 0) {
		panic("%s: vfs_add_dev: %e.\n", name, ret);
	}
	kprintf("partition %s: %d offset, %d sectors, %d KB\n", 
		name, pd->offset_sectors, pd->total_sectors, pd->total_sectors >> 1);
}

void dev_init_disk(void)
{
	kprintf("dev_init_disk() in\n");
	unsigned short ideno, devno;
	for (ideno = 0, devno = 0; ideno < MAX_IDE - 1; ideno++) {
		kprintf("loop for ideno: %d\n", ideno);
		if (ide_device_valid(ideno)) {
			disk_buffer[ideno] = kmalloc(DISK_BUFSIZE);
			if (disk_buffer[ideno] == NULL) {
				panic("disk%d alloc buffer failed.\n", ideno);
			}
			sem_init(&(disk_sem[ideno]), 1);

			ide_read_secs(ideno, 0, disk_buffer[ideno], 1);
			if (*(uint16_t*)(disk_buffer[ideno] + 510) != MBR_MAGIC) {
				kprintf("disk%d MBR invalid: 0x%04x\n", ideno, *(uint16_t*)(disk_buffer[ideno] + 510));
				continue;
			}

			struct partitiondesc *pd;
			int32_t pri_no;
			char name[] = "hda1\0";
			name[2] = 'a' + ideno;
			for (pri_no = 0; pri_no < 4; pri_no++) {
				pd = (struct partitiondesc*) (disk_buffer[ideno] + MBR_PTB_OFFSET + MBR_PTB_ENTRY_LEN * pri_no);
				if (pd->total_sectors <= 0)
					break;
				if (pd->valid == 0x80 || pd->valid == 0x00) {
					if (pd->type == 0x05 || pd->type == 0x0F) { //extend partition
						int32_t ext_no = 0, ext_offset = pd->offset_sectors, real_offset;
						//kprintf("ext_offset = %d\n", ext_offset);
						char *ext_buffer = kmalloc(DISK_BUFSIZE);
						while (pd->type == 0x05 || pd->type == 0x0F) {
							real_offset = pd->offset_sectors;
							ide_read_secs(ideno, pd->offset_sectors, ext_buffer, 1);
							pd = (struct partitiondesc*) (ext_buffer + MBR_PTB_OFFSET);
							//kprintf("no1: pd->offset = %d, ", pd->offset_sectors);
							pd->offset_sectors += real_offset;
							name[3] = '5' + ext_no;
							create_partition_dev(pd, ideno, devno++, name);
							pd = (struct partitiondesc*) (ext_buffer + MBR_PTB_OFFSET + MBR_PTB_ENTRY_LEN);
							//kprintf("no2: pd->offset = %d\n", pd->offset_sectors);
							pd->offset_sectors += ext_offset;
							ext_no++;
						}
						kfree(ext_buffer);
					}else{
						name[3] = '1' + pri_no;
						create_partition_dev(pd, ideno, devno++, name);
					}
				}else{
					kprintf("pd->valid incorrect: %d\n", pd->valid);
				}
			}
		}
	}

	if (ide_device_valid(RAMDISK_DEV_NO)) {
		struct partitiondesc ramdisk;
		ramdisk.offset_sectors = 0;
		ramdisk.total_sectors = ide_device_size(RAMDISK_DEV_NO) / DISK_BLK_NSECT * 8 / SECTSIZE;
		disk_buffer[RAMDISK_DEV_NO] = kmalloc(DISK_BUFSIZE);
		if (disk_buffer[RAMDISK_DEV_NO] == NULL) {
			panic("disk%d alloc buffer failed.\n", RAMDISK_DEV_NO);
		}
		sem_init(&(disk_sem[RAMDISK_DEV_NO]), 1);
		create_partition_dev(&ramdisk, RAMDISK_DEV_NO, devno, "ramdisk");
	}
}
