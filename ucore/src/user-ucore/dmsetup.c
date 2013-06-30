#include <ulib.h>
#include <stdio.h>
#include <mount.h>
#include <string.h>
#include <file.h>
#include <stat.h>
#include <unistd.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)

void usage(void)
{
	printf("usage: dmsetup create|remove source filename\n");
}


int read_data(int fd, char* buffer)
{
	int ret1;
	while ((ret1 = read(fd, buffer, sizeof(buffer))) > 0) {
		buffer += ret1;
	}
	return ret1;
}

int main(int argc, char **argv)
{
	int ret;
	
	static char buffer[4096];

	if (argc != 4) {
		usage();
		return -1;
	} else {

		if ((ret = open(argv[3], O_RDONLY)) < 0) {
			return ret;
		}
		if ((ret = read_data(ret, buffer)) != 0) {
			return ret;
		}
		ret = dmsetup(argv[1], argv[2], buffer);
	}

	if (!ret) {
		printf("%s %s.\n", argv[1], argv[2]);
	} else {
		printf("%s failed.\n", argv[1]);
	}

	return ret;
}
