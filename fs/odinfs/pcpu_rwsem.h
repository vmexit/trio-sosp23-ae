#ifndef _PER_CPU_OPT_RWSEM
#define _PER_CPU_OPT_RWSEM

#include <linux/atomic.h>
#include <linux/rwsem.h>
#include <linux/percpu.h>
#include <linux/wait.h>
#include <linux/lockdep.h>

#include "pmfs_config.h"

struct pcpu_rwsem {
	int __percpu 	*fastpath_count;
	atomic_t 	slowpath_count;
	atomic_t 	writers_count;
	wait_queue_head_t writers_wait_q;
	struct rw_semaphore rw_sem;
};

void pcpu_rwsem_down_read(struct pcpu_rwsem *);

int pcpu_rwsem_down_read_try_lock(struct pcpu_rwsem *pcpu_rwsem);

void pcpu_rwsem_up_read(struct pcpu_rwsem *);

void pcpu_rwsem_down_write(struct pcpu_rwsem *);

void pcpu_rwsem_up_write(struct pcpu_rwsem *);

int __pcpu_rwsem_init_rwsem(struct pcpu_rwsem *,
			    const char *, struct lock_class_key *);

void pcpu_rwsem_free_rwsem(struct pcpu_rwsem *);

#define pcpu_rwsem_init_rwsem(sem)					\
({								\
		static struct lock_class_key __key; 		\
	   __pcpu_rwsem_init_rwsem(sem, #sem, &__key); 		\
})



#endif
