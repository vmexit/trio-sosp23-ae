#ifndef _RING_BUFFER_PORTING_H_
#define _RING_BUFFER_PORTING_H_

#ifndef RING_BUFFER_CONF_KERNEL
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/mman.h>
# include <sys/wait.h>
# include <fcntl.h>
# include <unistd.h>
# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <string.h>
# include <assert.h>
# define __STDC_FORMAT_MACROS
# include <inttypes.h>
# include <sys/mman.h>
# include <sched.h>
# include <pthread.h>
# include <errno.h>
# include <arch.h>
#include "shm_sync.h"
#else
# include <linux/kernel.h>
# include <linux/kthread.h>
# include <linux/string.h>
# include <linux/vmalloc.h>
# include <linux/slab.h>
# include <linux/module.h>
# include <linux/wait.h>
# include <asm/io.h>
# include <asm/cacheflush.h>
# include <asm/pgtable.h>
# include <asm/tlbflush.h>
# include <asm/pgalloc.h>
#endif /* RING_BUFFER_CONF_KERNEL */

#ifdef __cplusplus
extern "C" {
#endif

struct mmap_info_t;

#ifndef RING_BUFFER_CONF_KERNEL
static inline
void *__rb_memcpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

static inline
void *__rb_malloc(size_t size)
{
        return malloc(size);
}

static inline
void __rb_free(void *ptr)
{
        free(ptr);
}

static inline
void *__rb_calloc(size_t nmemb, size_t size)
{
        return calloc(nmemb, size);
}

static inline
int __rb_mutex_init(const char* name, struct shmmtx_t *mutex)
{
	return shmmtx_open(name, &mutex);
}

static inline
int __rb_mutex_destroy(struct shmmtx_t *mutex)
{
	return shmmtx_close(mutex);
}

static inline
int __rb_mutex_lock(struct shmmtx_t *mutex)
{
	return pthread_mutex_lock(mutex->mtx);
}

static inline
int __rb_mutex_unlock(struct shmmtx_t *mutex)
{
	return pthread_mutex_unlock(mutex->mtx);
}

static inline
int __rb_wait_init(const char* name, struct shmcv_t *wait)
{
	return shmcv_open(name, &wait);
}

static inline
int __rb_wait_destroy(struct shmcv_t *wait)
{
	return shmcv_close(wait);
}

static inline
int __rb_wait_wake_up(struct shmcv_t *wait)
{
	return pthread_cond_signal(wait->cv);
}

static inline
int __rb_wait_wake_up_all(struct shmcv_t *wait)
{
	return pthread_cond_broadcast(wait->cv);
}

static inline
int __rb_wait_sleep_on(struct shmcv_t *wait, struct shmmtx_t *mutex)
{
	return pthread_cond_wait(wait->cv, mutex->mtx);
}

static inline
void __rb_yield(void)
{
	smp_mb();
	sched_yield();
	smp_rmb();
}

#else /* RING_BUFFER_CONF_KERNEL */
static inline
void *__rb_memcpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

static inline
void *__rb_malloc(size_t size)
{
        return kmalloc(size, GFP_KERNEL);
}

static inline
void __rb_free(void *ptr)
{
        kfree(ptr);
}

static inline
void *__rb_calloc(size_t nmemb, size_t size)
{
        return kzalloc(nmemb * size, GFP_KERNEL);
}

static inline
int __rb_mutex_init(struct mutex *mutex)
{
	mutex_init(mutex);
	return 0;
}

static inline
int __rb_mutex_destroy(struct mutex *mutex)
{
	mutex_destroy(mutex);
	return 0;
}

static inline
int __rb_mutex_lock(struct mutex *mutex)
{
	mutex_lock(mutex);
	return 0;
}

static inline
int __rb_mutex_unlock(struct mutex *mutex)
{
	mutex_unlock(mutex);
	return 0;
}

static inline
int __rb_wait_init(wait_queue_head_t *wait)
{
	init_waitqueue_head(wait);
	return 0;
}

static inline
int __rb_wait_destroy(wait_queue_head_t *wait)
{
	return 0;
}

static inline
int __rb_wait_wake_up(wait_queue_head_t *wait)
{
	wake_up(wait);
	return 0;
}

static inline
int __rb_wait_wake_up_all(wait_queue_head_t *wait)
{
	wake_up_all(wait);
	return 0;
}

#define __rb_wait_sleep_on(wq_head, condition) \
		wait_event_interruptible(wq_head, condition)

static inline
void __rb_yield(void)
{
	smp_mb();
	if (need_resched())
		cond_resched();
	else
		schedule();
	smp_rmb();
}

#endif /* RING_BUFFER_CONF_KERNEL */

#ifdef __cplusplus
}
#endif
#endif /* _RING_BUFFER_PORTING_H_ */
