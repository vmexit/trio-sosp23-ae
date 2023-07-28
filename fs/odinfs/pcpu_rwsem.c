#include <linux/atomic.h>
#include <linux/rwsem.h>
#include <linux/percpu.h>
#include <linux/wait.h>
#include <linux/lockdep.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/errno.h>

#include "pcpu_rwsem.h"

int __pcpu_rwsem_init_rwsem(struct pcpu_rwsem *pcpu_rwsem,
			    const char *name,
			    struct lock_class_key *rw_sem_key)
{
	pcpu_rwsem->fastpath_count = alloc_percpu(int);
	if (unlikely(!pcpu_rwsem->fastpath_count))
		return -ENOMEM;
	__init_rwsem(&pcpu_rwsem->rw_sem, name, rw_sem_key);
	atomic_set(&pcpu_rwsem->writers_count, 0);
	atomic_set(&pcpu_rwsem->slowpath_count, 0);
	init_waitqueue_head(&pcpu_rwsem->writers_wait_q);
	return 0;
}

void pcpu_rwsem_free_rwsem(struct pcpu_rwsem *pcpu_rwsem)
{
	free_percpu(pcpu_rwsem->fastpath_count);
	pcpu_rwsem->fastpath_count = NULL;
}

static inline bool try_fastpath(struct pcpu_rwsem *pcpu_rwsem, int val)
{
	bool fastpath = false;
	preempt_disable();
	if (likely(!atomic_read(&pcpu_rwsem->writers_count))) {
		this_cpu_add(*pcpu_rwsem->fastpath_count, val);
		fastpath = true;
	}
	preempt_enable();
	return fastpath;
}

static inline void take_slowpath(struct pcpu_rwsem *pcpu_rwsem)
{
	down_read(&pcpu_rwsem->rw_sem);
	atomic_inc(&pcpu_rwsem->slowpath_count);
	up_read(&pcpu_rwsem->rw_sem);
}

void pcpu_rwsem_down_read(struct pcpu_rwsem *pcpu_rwsem)
{
	if (likely(try_fastpath(pcpu_rwsem, 1)))
		return;

	take_slowpath(pcpu_rwsem);
}

int pcpu_rwsem_down_read_try_lock(struct pcpu_rwsem *pcpu_rwsem)
{
	if (likely(try_fastpath(pcpu_rwsem, 1)))
		return 1;

	if (down_read_trylock(&pcpu_rwsem->rw_sem)) {
		atomic_inc(&pcpu_rwsem->slowpath_count);
		up_read(&pcpu_rwsem->rw_sem);
		return 1;
	}

	return 0;
}

void pcpu_rwsem_up_read(struct pcpu_rwsem *pcpu_rwsem)
{
	if (likely(try_fastpath(pcpu_rwsem, -1)))
		return;

	if (atomic_dec_and_test(&pcpu_rwsem->slowpath_count))
		wake_up_all(&pcpu_rwsem->writers_wait_q);
}

static int clear_fastpath(struct pcpu_rwsem *pcpu_rwsem)
{
	int sum = 0;
	int cpu;

	for_each_possible_cpu(cpu) {
		sum += per_cpu(*pcpu_rwsem->fastpath_count, cpu);
		per_cpu(*pcpu_rwsem->fastpath_count, cpu) = 0;
	}
	return sum;
}

void pcpu_rwsem_down_write(struct pcpu_rwsem *pcpu_rwsem)
{
	atomic_inc(&pcpu_rwsem->writers_count);

	/*
	 * This line is there in the original source code but I comment it here.
	 * This causes a major performance hit and it looks to me not needed; there
	 * is no RCU used in this rwsem
	 *
	 *  synchronize_rcu_expedited();
	 */
	down_write(&pcpu_rwsem->rw_sem);
	atomic_add(clear_fastpath(pcpu_rwsem), &pcpu_rwsem->slowpath_count);
	wait_event(pcpu_rwsem->writers_wait_q, !atomic_read(&pcpu_rwsem->slowpath_count));
}

void pcpu_rwsem_up_write(struct pcpu_rwsem *pcpu_rwsem) {
	up_write(&pcpu_rwsem->rw_sem);

    /*
     * This line is there in the original source code but I comment it here.
     * This causes a major performance hit and it looks to me not needed; there
     * is no RCU used in this rwsem
     *
     *  synchronize_rcu_expedited();
     */
	atomic_dec(&pcpu_rwsem->writers_count);
}
