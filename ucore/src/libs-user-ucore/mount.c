#include "mount.h"
#include <syscall.h>

int
mount(const char *source, const char *target, const char *filesystemtype,
      const void *data)
{
	return sys_mount(source, target, filesystemtype, data);
}

int umount(const char *target)
{
	return sys_umount(target);
}

int dmsetup(const char *cmd, const char *target, const char *rule)
{
	return sys_dmsetup(cmd, target, rule);
}

int mksfs(const char *dev_name)
{
	return sys_mksfs(dev_name);
}

