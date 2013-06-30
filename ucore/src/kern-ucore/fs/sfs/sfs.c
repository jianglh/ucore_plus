#include <types.h>
#include <sfs.h>
#include <error.h>
#include <assert.h>
#include <vfs.h>
#include <string.h>

extern struct device* no2dev[128];
extern char* bootparm;
char* bootdevice;
void sfs_init(void)
{
	int ret;
	if ((ret = register_filesystem("sfs", sfs_mount)) != 0) {
		panic("failed: sfs: register_filesystem: %e.\n", ret);
	}
	//kprintf("sfs_init: 0x%08x, 0x%08x\n", (int)no2dev[0], (int)no2dev[1]);
	//sfs_mksfs(no2dev[1]);

	bootdevice = strdup("ramdisk");
	if (bootparm != NULL) {
		kprintf("bootparm: %s\n", bootparm);
		int32_t len = strlen(bootparm), i;
		for (i = 0; i + 5 < len; i++)
			if (strncmp(bootparm + i, "root=", 5) == 0) {
				int ed = i + 5;
				while (ed < len && *(bootparm + ed) != ' ')
					ed++;
				strncpy(bootdevice, bootparm + i + 5, ed - i - 5);
				bootdevice[ed - i - 5] = '\0';
				kprintf("specfied boot device: %s\n", bootdevice);
			}
	}
	if ((ret = sfs_mount(bootdevice)) != 0) {
		panic("failed: sfs: sfs_mount: %e.\n", ret);
	}
}
