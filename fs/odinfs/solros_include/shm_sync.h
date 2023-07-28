#ifndef SPMFS_SHM_SYNC_H_
#define SPMFS_SHM_SYNC_H_

#include <pthread.h>

struct shmmtx_t {
	char *name;
	pthread_mutex_t *mtx;
	int fd;
	int init;
};

struct shmcv_t {
	char *name;
	pthread_cond_t *cv;
	int fd;
	int init;
};

int shmmtx_open(const char *name, struct shmmtx_t **shmmtx);
int shmmtx_close(struct shmmtx_t *shmmtx);
int shmcv_open(const char *name, struct shmcv_t **shmcv);
int shmcv_close(struct shmcv_t *shmcv);

#endif /* SPMFS_SHM_SYNC_H_ */