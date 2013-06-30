#include <types.h>
#include <mmu.h>
#include <slab.h>
#include <sem.h>
#include <ide.h>
#include <inode.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <error.h>
#include <assert.h>


#define MAPPER_BLKSIZE                   PGSIZE
#define MAPPER_BLK_NSECT                 (MAPPER_BLKSIZE / SECTSIZE)


static int mapper_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int mapper_close(struct device *dev)
{
	return 0;
}

static int min(size_t a, size_t b)
{
	return a<b?a:b;
}

static int mapper_io(struct device *dev, struct iobuf *iob, bool write)
{
	off_t offset = iob->io_offset;
	size_t resid = iob->io_resid, finish = iob->io_offset + iob->io_resid;
	uint32_t blkno = offset / MAPPER_BLKSIZE;
	uint32_t nblks = resid / MAPPER_BLKSIZE;

	if ((offset % MAPPER_BLKSIZE) != 0 || (resid % MAPPER_BLKSIZE) != 0) {
		return -E_INVAL;
	}

	if (blkno + nblks > dev->d_blocks) {
		return -E_INVAL;
	}

	
	if (nblks == 0) {
		return 0;
	}


	struct iobuf temp_iob;

	while (offset != finish)
	{
		struct dev_list *p = dev->map_list;
		while (p)
		{
			if (SECTSIZE * (p->start_sector) <= offset && SECTSIZE * (p->start_sector + p->len) > offset) break;
			p = p->next;
		}
		temp_iob.io_base = iob->io_base + (offset - iob->io_offset);
		temp_iob.io_offset = (offset - p->start_sector) + p->offset;


		temp_iob.io_len = temp_iob.io_resid = min(finish - offset, SECTSIZE * (p->start_sector + p->len) - offset);
		offset += temp_iob.io_len;\
		p->dev->d_io(p->dev, &temp_iob, write);
	}
	return 0;
}

static int mapper_ioctl(struct device *dev, int op, void *data)
{
	return -E_UNIMP;
}

static void mapper_device_init(struct device *dev, const char* path)
{
	memset(dev, 0, sizeof(*dev));
	static_assert(MAPPER_BLKSIZE % SECTSIZE == 0);


	struct dev_list *now_point = NULL;
	int i;
	for (i = 0; path[i] != '\0'; i++)
	{
		size_t p1 = 0, p2 = 0, p3 = 0, j = 0;
		char dev_name[10];


		while (path[i]!=' ' && path[i]!='\n')
			p1 = p1 * 10 + path[i++] - '0';
		if (path[i++] == '\n') break;

		while (path[i]!=' ' && path[i]!='\n')
			p2 = p2 * 10 + path[i++] - '0';
		if (path[i++] == '\n') break;
		
		while (path[i]!=' ' && path[i]!='\n') i++;
		if (path[i++] == '\n') break;

		while (path[i]!=' ' && path[i]!='\n')
			dev_name[j++] = path[i++];
		dev_name[j] = '\0';
		if (path[i++] == '\n') break;

		while (path[i]!=' ' && path[i]!='\n')
			p3 = p3 * 10 + path[i++] - '0';

		struct dev_list *temp = kmalloc(sizeof(struct dev_list));
		temp -> next = now_point;
		temp -> start_sector = p1;
		temp -> len = p2;
		temp -> offset = p3;
		dev->d_total_sectors += temp -> len;

		if ((p1 % MAPPER_BLK_NSECT) != 0 || (p2 % MAPPER_BLK_NSECT) != 0 || (p3 % MAPPER_BLK_NSECT) != 0)
		{
			return -E_INVAL;
		}

		kprintf(" %d %d %d %s\n", p1, p2 ,p3, dev_name);

		struct inode *node_store;
		
		int ret = find_dev(dev_name, &node_store);

		struct device *dev = vop_info(node_store, device);

		temp -> dev = dev;
		now_point = temp;
	}

	dev->map_list = now_point;
	dev->d_ideno = DEVICE_MAPPER_DEV_NO;
	dev->d_devno = 1000;
	dev->d_offset_sectors = 0;
	dev->d_blocks = dev->d_total_sectors / MAPPER_BLK_NSECT;
	dev->d_blocksize = MAPPER_BLKSIZE;
	dev->d_open = mapper_open;
	dev->d_close = mapper_close;
	dev->d_io = mapper_io;
	dev->d_ioctl = mapper_ioctl;

}

void dev_init_mapper(const char* name, const char* path)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("%s: dev_create_node.\n", name);
	}
	mapper_device_init(vop_info(node, device), path);

	int ret;
	if ((ret = vfs_add_dev(name, node, 1)) != 0) {
		panic("%s: vfs_add_dev: %e.\n", name, ret);
	}
}

void dev_remove_mapper(const char* name)
{
	int ret;
	if ((ret = vfs_dec_dev(name)) != 0) {
		kprintf("%s: vfs_add_dev: %e.\n", name, ret);
	}
}
