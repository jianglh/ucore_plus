/*-----------------------------------------------------------------------
/  Low level disk interface modlue include file
/-----------------------------------------------------------------------*/  

#include <kio.h>
#include <fs.h>
#include <ide.h>
#include <inode.h>
#include "diskio.h"
#include <dev.h>

extern struct device* no2dev[128];

/*---------------------------------------*/ 
/* Prototypes for disk control functions */ 

int assign_drives(int st, int ed)
{
	//FAT_PRINTF("[FATFS], assign %d to %d\n", st, ed);
	return 1;
}

DSTATUS disk_initialize(BYTE drive)
{
	//FAT_PRINTF("[FATFS], disk_init on drive%d\n", drive);
	return 0;
}


DSTATUS disk_status(BYTE drive)
{
	//FAT_PRINTF("[FATFS], disk_status on drive%d\n", drive);
	return 0;
}


DRESULT disk_read(BYTE drive, BYTE * buffer, DWORD sectorNumber, BYTE sectorCount)
{

	struct device * dev = no2dev[drive];
	if (NULL == dev)
		panic("no2dev doesn't work\n");

    //FAT_PRINTF("[FATFS], disk_read on drive%d\n", drive);
    //from dev_diso0.c read_blks
	int ret;
	//kprintf("disk_read ## %d\n", drive);
	
	if ((ret = ide_read_secs(dev->d_ideno, dev->d_offset_sectors + sectorNumber, buffer, sectorCount)) != 0) {
		panic("fat: read blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
			-1, sectorNumber, 0, sectorCount, ret);
	}
	
	return 0;
}


DRESULT disk_write(BYTE drive, const BYTE * buffer, DWORD sectorNumber, BYTE sectorCount)
{
	struct device * dev = no2dev[drive];
	if (NULL == dev)
		panic("no2dev doesn't work\n");

	//FAT_PRINTF("[FATFS], disk_write on drive%d\n", drive);
	int ret;
	if ((ret = ide_write_secs(dev->d_ideno, dev->d_offset_sectors + sectorNumber, buffer, sectorCount)) != 0) {
		panic("fat: write blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
			-1, sectorNumber, 0, sectorCount, ret);
	}
	return 0;
}

DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
	//FAT_PRINTF("[FATFS], disk_ioctl on drive%d, command = %d\n", drive, command);
	return 0;
}




