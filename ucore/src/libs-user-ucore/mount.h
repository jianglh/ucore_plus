#ifndef __USER_LIBS_MOUNT_H__
#define __USER_LIBS_MOUNT_H__

int mount(const char *source, const char *target, const char *filesystemtype,
	  const void *data);
int umount(const char *target);

int dmsetup(const char *cmd, const char *target, const char *rule);

int mksfs(const char *dev_name);

#endif
