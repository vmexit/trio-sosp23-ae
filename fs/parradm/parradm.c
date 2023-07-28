#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "pmem_ar.h"


int main(int argc, char * argv[])
{
	int fd = 0;
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s create/delete <path_to_disk1> "
				"<path_to_disk2> ...\n", argv[0]);

		exit(1);
	}

	fd = open("/dev/"SPMFS_DEV_INSTANCE_NAME, O_RDWR);

	if (fd == -1)
		perror("open");

	/* allows shorthand cr */
	if (strncmp(argv[1], "cr", 2) == 0)
	{
		struct pmem_arg_info pmem_arg_info;
		int i = 0;

		pmem_arg_info.num = argc - 2;

		for (i = 0; i < pmem_arg_info.num; i++)
			pmem_arg_info.paths[i] = argv [i + 2];

		if (ioctl(fd, SPMFS_PMEM_AR_CMD_CREATE, &pmem_arg_info) == -1)
		{
			perror("ioctl: create");
			exit(1);
		}
	}
	/* allows shorthand de */
	else if (strncmp(argv[1], "de", 2) == 0)
	{
		if (ioctl(fd, SPMFS_PMEM_AR_CMD_DELETE) == -1)
		{
			perror("ioctl: delete");
			exit(1);
		}
	}
	else
	{
		fprintf(stderr, "Unknown cmd: %s\n", argv[1]);
		exit(1);
	}

	return 0;
}
