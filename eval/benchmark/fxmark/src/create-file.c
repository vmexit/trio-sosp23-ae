#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "cpupol.h"

#define BUF_SIZE 1048576

char * file_name_g = NULL;
unsigned long sz_g = 0;

static char buf[BUF_SIZE];

static int setaffinity(int c)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(c, &cpuset);
	return sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

static int create_file(char *path, long long size, int cpu)
{
	int fd, rc;

	char fname[256];
	sprintf(fname, "%s.%d.0", path, cpu);

	/* delete the file */
	unlink(fname);
#if 0
	printf("create files :%s at core: %d\n", fname, seq_cores[cpu]);
#endif

	fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

	if (fd == -1) {
		perror("Error while opening file");
		return 1;
	}

	while (size > 0) {
		if (size < BUF_SIZE)
			rc = write(fd, buf, size);
		else
			rc = write(fd, buf, BUF_SIZE);

		if (rc == -1) {
			perror("Error while writing file");
			return 1;
		}

		size -= rc;
	}

	close(fd);

	return 0;
}

void * func(void * arg)
{
    unsigned long i = (unsigned long) arg;

    setaffinity(seq_cores[i]);
    create_file(file_name_g, sz_g, i);

    return NULL;
}

static int create(int ncpu)
{
    pthread_t * thread = NULL;
    int i = 0;

    thread = malloc(sizeof(pthread_t) * ncpu);

	for (i = 0; i < ncpu; i++)
	{
	    pthread_create(&thread[i], NULL, func, (void *) i);

	}

	for (i = 0; i < ncpu; i++)
	    pthread_join(thread[i], NULL);

	return 0;
}

/* remove core in place */
static void remove_core(unsigned int *cores, int len, int core)
{
	int idx, i;
	for (idx = 0; idx < len; ++idx) {
		if (cores[idx] == core) {
			break;
		}
	}
	for (i = idx; i < len - 1; ++i) {
		cores[i] = cores[i + 1];
	}
}

int main(int argc, char **argv)
{
	if (argc != 6) {
		fprintf(stderr,
			"%s: [path] [size] [ncpu] [delegation_threads] [delegation_sockets]\n",
			argv[0]);
		return 1;
	}

	file_name_g = argv[1];
	sz_g = atoll(argv[2]);

	int ncpu = atoi(argv[3]);
	int delegation_threads = atoi(argv[4]);
	int delegation_sockets = atoi(argv[5]);
	assert(sz_g > 0);
	assert(ncpu > 0);

	int total_cpus = PHYSICAL_CHIPS * CORE_PER_CHIP;
	/* change ncore and seq_cores to adapt odinfs */
	if (delegation_threads > 0) {
		for (int i = 0; i < delegation_sockets; ++i) {
			for (int j = 0; j < delegation_threads; ++j) {
				int core = i * CORE_PER_CHIP + j;
				remove_core(seq_cores, total_cpus, core);
			}
		}
	}

	return create(ncpu);
}
