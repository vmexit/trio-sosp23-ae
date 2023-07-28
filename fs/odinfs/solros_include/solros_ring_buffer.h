#ifndef _SOLROS_RING_BUFFER_H_
#define _SOLROS_RING_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/limits.h>
#include <arch.h>
#include <solros_ring_buffer_common.h>

#ifndef RING_BUFFER_CONF_KERNEL
#include <shm_sync.h>
#endif

/*
 * configurations for lock-based version
 */
/* #define RING_BUFFER_TWO_LOCK */
/* #define RING_BUFFER_TWO_LOCK_TICKETLOCK */
/* #define RING_BUFFER_TWO_LOCK_MCSLOCK */

#ifdef  RING_BUFFER_TWO_LOCK
# if defined(RING_BUFFER_TWO_LOCK_TICKETLOCK)
typedef struct ticketlock_t spinlock_t;
# elif defined(RING_BUFFER_TWO_LOCK_MCSLOCK)
typedef struct mcslock_t spinlock_t;
# endif
#endif /* RING_BUFFER_TWO_LOCK */

/*
 * ring buffer object
 */
#define RING_BUFFER_NAME_MAX        128

struct solros_ring_buffer_t;
struct solros_ring_buffer_elm_t;
struct solros_ring_buffer_req_t;

typedef int  (*solros_ring_buffer_is_healthy_t)(struct solros_ring_buffer_t *);

struct solros_ring_buffer_nap_info_t {
	volatile int                       is_nap_time;
	int                                monitoring_status;
	volatile struct solros_ring_buffer_elm_t *monitoring_elm;
#ifndef RING_BUFFER_CONF_KERNEL
	struct shmmtx_t                    mutex;
	struct shmcv_t                     wait;
#else
	struct mutex                       mutex;
	wait_queue_head_t                  wait;
#endif /* RING_BUFFER_CONF_KERNEL */
};

struct solros_ring_buffer_t {
	size_t size ____cacheline_aligned2;         /* in bytes */
	size_t align_mask;                          /* data alignment mask */
	void *buff;                                 /* start of ring buffer */
	int  is_blocking;                           /* blocking or non-blocking */
	solros_ring_buffer_reap_cb_t reap_cb;              /* user-defined reap callback */
	void* reap_cb_arg;                          /* user-defined reap callback argument */
	solros_ring_buffer_is_healthy_t is_healthy;        /* health check function */
	void *private_value;                        /* any value by its user */

	volatile size_t head  ____cacheline_aligned2; /* byte offset */
	volatile size_t tail2;                      /* byte offset */

	volatile size_t tail  ____cacheline_aligned2; /* byte offset */

#ifndef RING_BUFFER_TWO_LOCK
	volatile struct solros_ring_buffer_req_t *put_req ____cacheline_aligned2;

	volatile struct solros_ring_buffer_req_t *get_req ____cacheline_aligned2;
#else
	spinlock_t put_lock  ____cacheline_aligned2;
	spinlock_t get_lock  ____cacheline_aligned2;
#endif  /* RING_BUFFER_TWO_LOCK */

	struct solros_ring_buffer_nap_info_t      put_nap ____cacheline_aligned2;
	struct solros_ring_buffer_nap_info_t      get_nap ____cacheline_aligned2;

	char name[RING_BUFFER_NAME_MAX];            /* name for debugging */
} ____cacheline_aligned2;

/*
 * ring buffer API
 */
int  __solros_ring_buffer_create(const char *where, unsigned int line, const char *var,
			  const char* shm_name, char* mutex_name, char* cv_name, size_t size_hint, size_t align,
			  size_t socket_id, int is_open, int is_blocking, solros_ring_buffer_reap_cb_t reap_cb,
			  void* reap_cb_arg, struct solros_ring_buffer_t **prb);

#define solros_ring_buffer_create(shm_name, mutex_name, cv_name, size_hint, align, socket_id, is_open, is_blocking, reap_cb, reap_cb_arg, prb) \
	__solros_ring_buffer_create(__func__, __LINE__, #prb,			\
			     shm_name, mutex_name, cv_name, size_hint, align, socket_id, is_open, is_blocking,		\
			     reap_cb, reap_cb_arg, prb)
void solros_ring_buffer_destroy(struct solros_ring_buffer_t *rb, int is_open, const char *shm_name);

int  solros_ring_buffer_put(struct solros_ring_buffer_t *rb, struct solros_ring_buffer_req_t *req);
int  solros_ring_buffer_get(struct solros_ring_buffer_t *rb, struct solros_ring_buffer_req_t *req);
int  solros_ring_buffer_put_nolock(struct solros_ring_buffer_t *rb, struct solros_ring_buffer_req_t *req);
int  solros_ring_buffer_get_nolock(struct solros_ring_buffer_t *rb, struct solros_ring_buffer_req_t *req);

void solros_ring_buffer_elm_set_ready(struct solros_ring_buffer_t *rb, void *data);
void solros_ring_buffer_elm_set_done(struct solros_ring_buffer_t *rb, void *data);
int  solros_ring_buffer_elm_valid(struct solros_ring_buffer_t *rb, void *data);

int  solros_copy_from_ring_buffer(struct solros_ring_buffer_t *rb,
			   void *dest_mem, const void *src_rb, size_t n);
int  solros_copy_to_ring_buffer(struct solros_ring_buffer_t *rb,
			 void *dest_rb, const void *src_mem, size_t n);
unsigned int solros_ring_buffer_get_compat_vector(void);
int    solros_ring_buffer_is_empty(struct solros_ring_buffer_t *rb);
int    solros_ring_buffer_is_full(struct solros_ring_buffer_t *rb);
size_t solros_ring_buffer_free_space(struct solros_ring_buffer_t *rb);
size_t solros_ring_buffer_secure_free_space(struct solros_ring_buffer_t *rb, size_t n);
#ifdef __cplusplus
}
#endif
#endif /* _RING_BUFFER_H_ */
