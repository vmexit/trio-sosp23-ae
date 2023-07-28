#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shm_sync.h"

int shmmtx_open(const char *name, struct shmmtx_t **shmmtx)
{
	int rc;
	int fd;
	int init = 0;
	pthread_mutexattr_t attr;
	pthread_mutex_t *mtx;
	struct shmmtx_t *shm_mutex;

	/* shm_open: open the backing file in shared memory */
	fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0660);
	if (fd == -1) {
		if (errno != EEXIST) {
			perror("shm_open");
			goto error;
		}
		/* try to open the shared memory region instead */
		fd = shm_open(name, O_RDWR, 0660);
		if (fd == -1) {
			perror("shm_open");
			goto error;
		}

		/* mmap: allocate mutex structure */
		mtx = mmap(NULL, sizeof(pthread_mutex_t),
			   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (mtx == MAP_FAILED) {
			perror("mmap");
			goto error;
		}
	} else {
		/* ftruncate: resize the shared memory buffer */
		rc = ftruncate(fd, sizeof(pthread_mutex_t));
		if (rc != 0) {
			perror("ftruncate");
			goto error;
		}

		/* mmap: allocate mutex structure */
		mtx = mmap(NULL, sizeof(pthread_mutex_t),
			   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (mtx == MAP_FAILED) {
			perror("mmap");
			goto error;
		}

		/* initialize the attribute */
		rc = pthread_mutexattr_init(&attr);
		if (rc) {
			perror("pthread_mutexattr_init");
			goto error;
		}

		/* set the mutex attritube PTHREAD_PROCESS_SHARED to permit a mutex to
		 * be operated upon by any thread that has access to the memory where
		 * the mutex is allocated
		 */
		rc = pthread_mutexattr_setpshared(&attr,
						  PTHREAD_PROCESS_SHARED);
		if (rc) {
			perror("pthread_mutexattr_setpshared");
			goto error;
		}

		/* initialize the mutex */
		rc = pthread_mutex_init(mtx, &attr);
		if (rc) {
			perror("pthread_mutex_init");
			goto error;
		}
	}

	shm_mutex = malloc(sizeof(struct shmmtx_t));
	if (!shm_mutex)
		goto error;

	shm_mutex->name = strdup(name);
	shm_mutex->fd = fd;
	shm_mutex->mtx = mtx;
	shm_mutex->init = init;

	memcpy(*shmmtx, shm_mutex, sizeof(struct shmmtx_t));

	return 0;

error:
	shm_unlink(name);
	if (fd != -1)
		close(fd);
	shmmtx = NULL;

	return -1;
}

int shmmtx_close(struct shmmtx_t *shmmtx)
{
	int rc;

	assert(shmmtx);
	/* pthread_mutex_destroy: free the mutex */
	rc = pthread_mutex_destroy(shmmtx->mtx);
	if (rc) {
		perror("pthread_mutex_destroy");
		goto error;
	}

	/* munmap: deallocate the memory */
	rc = munmap(shmmtx->mtx, sizeof(pthread_mutex_t));
	if (rc) {
		perror("munmap");
		goto error;
	}

	/* close: close the backing file for shared memory */
	rc = close(shmmtx->fd);
	if (rc) {
		perror("close");
		goto error;
	}

	/* shm_unlink: unlink the shared memory */
	rc = shm_unlink(shmmtx->name);
	if (rc < 0) {
		perror("shm_unlink");
		goto error;
	}

	if (shmmtx->name) {
		free(shmmtx->name);
	}

	shmmtx->mtx = NULL;
	shmmtx->name = NULL;

error:
	return -1;
}

int shmcv_open(const char *name, struct shmcv_t **shmcv)
{
	int rc;
	int fd;
	void *addr;
	int init = 0;
	pthread_condattr_t attr;
	pthread_cond_t *cond;
	struct shmcv_t *shm_cv;

	/* shm_open: open the backing file in shared memory */
	fd = shm_open(name, O_RDWR, 0660);

	/* shm_open: create the backing file in shared memory */
	if (errno == ENOENT) {
		fd = shm_open(name, O_RDWR | O_CREAT, 0660);
		init = 1;
	}

	if (fd == -1) {
		perror("shm_open");
		goto error;
	}

	/* ftruncate: resize the shared memory buffer */
	rc = ftruncate(fd, sizeof(pthread_cond_t));
	if (rc != 0) {
		perror("ftruncate");
		goto error;
	}

	/* mmap: allocate conditional variable structure */
	addr = mmap(NULL, sizeof(pthread_cond_t), PROT_READ | PROT_WRITE,
		    MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		goto error;
	}

	cond = (pthread_cond_t *)addr;

	/* initialize the newly created cv */
	if (init) {
		/* initialize the attribute */
		rc = pthread_condattr_init(&attr);
		if (rc) {
			perror("pthread_condattr_init");
			goto error;
		}

		/* set the cv attritube PTHREAD_PROCESS_SHARED to permit a cv to
		 * be operated upon by any thread that has access to the memory where
		 * the cv is allocated
		 */
		rc = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		if (rc) {
			perror("pthread_condattr_setpshared");
			goto error;
		}

		/* initialize the cv */
		rc = pthread_cond_init(cond, &attr);
		if (rc) {
			perror("pthread_cond_init");
			goto error;
		}
	}

	shm_cv = malloc(sizeof(struct shmcv_t));
	if (!shm_cv)
		goto error;

	shm_cv->name = strdup(name);
	shm_cv->fd = fd;
	shm_cv->cv = cond;
	shm_cv->init = init;

	memcpy(*shmcv, shm_cv, sizeof(struct shmcv_t));

	return 0;

error:
	shm_unlink(name);
	if (fd != -1)
		close(fd);
	shmcv = NULL;

	return -1;
}

int shmcv_close(struct shmcv_t *shmcv)
{
	int rc;

	assert(shmcv);
	/* pthread_cond_destroy: free the conditional variable */
	rc = pthread_cond_destroy(shmcv->cv);
	if (rc) {
		perror("pthread_cond_destroy");
		goto error;
	}

	/* munmap: deallocate the memory */
	rc = munmap(shmcv->cv, sizeof(pthread_cond_t));
	if (rc) {
		perror("munmap");
		goto error;
	}

	/* close: close the backing file for shared memory */
	rc = close(shmcv->fd);
	if (rc) {
		perror("close");
		goto error;
	}

	/* shm_unlink: unlink the shared memory */
	rc = shm_unlink(shmcv->name);
	if (rc < 0) {
		perror("shm_unlink");
		goto error;
	}

	if (shmcv->name) {
		free(shmcv->name);
	}

	shmcv->cv = NULL;
	shmcv->name = NULL;

error:
	return -1;
}