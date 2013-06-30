#include <ulib.h>
#include <stdio.h>
#include <mount.h>
#include <string.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)

void usage(void)
{
	printf("usage: mksfs device\n");
}

int main(int argc, char **argv)
{
	int ret;
	if (argc != 2) {
		usage();
		return -1;
	} else {
		ret = mksfs(argv[1]);
	}

	if (!ret) {
		printf("mksfs %s.\n", argv[1]);
	} else {
		printf("mksfs failed.\n");
	}

	return ret;
}
