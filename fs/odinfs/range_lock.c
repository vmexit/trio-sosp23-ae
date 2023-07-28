#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io.h>

#include "range_lock.h"
#include "pmfs_config.h"

#ifdef RANGE_LOCK_SEGMENT

#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
static DEFINE_PER_CPU(u32, check_bias);

static inline uint32_t mix32a(uint32_t v)
{
    static const uint32_t mix32ka = 0x9abe94e3 ;
    v = (v ^ (v >> 16)) * mix32ka ;
    v = (v ^ (v >> 16)) * mix32ka ;
    return v;
}

static inline uint32_t hash(uint64_t addr) {
    return mix32a((uint64_t)current ^ addr) % NUM_SLOT;
}
#endif

struct range_lock *range_lock_init(unsigned long size)
{
	struct range_lock *lock = kzalloc(sizeof(struct range_lock), GFP_KERNEL);
	BUG_ON(!lock);

	pmfs_init_range_lock(lock);
	BUG_ON(!(lock->sg_table));
		
	return lock;
}

void range_lock_free(struct range_lock *lock)
{
#ifdef PMFS_ENABLE_RANGE_LOCK_KMEM_CACHE
	if(lock->sg_size == SEGMENT_INIT_COUNT)
	       kmem_cache_free(pmfs_rangelock_cachep, lock->sg_table);
	else
		vfree(lock->sg_table);
#else	
	vfree(lock->sg_table);
#endif
#if defined(RGLOCK_BRAVO)
	vfree(lock->vr_table);
#endif
}

void range_read_lock(struct range_lock *lock, unsigned long start, unsigned long size)
{
	unsigned long trials;
	unsigned int wlock = (unsigned int)1 << 31;
	unsigned long i, end = start + size;
	int old;
	struct sg_entry *e;


	for (i = start; i < end; ++i) {
		trials = 0;
		e = &lock->sg_table[i];

#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
		if (READ_ONCE(e->rbias)) {
			volatile atomic_t **slot = NULL;
#if defined(RGLOCK_BRAVO)
			u32 id = hash((uint64_t)(&e->segment));
			slot = &lock->vr_table[V(id)];
#elif defined(GLOBAL_RGLOCK_BRAVO)
			u32 id = hash((uint64_t)(&e->segment));
			slot = &global_vr_table[V(id)];
#endif
			if (cmpxchg(slot, NULL, &e->segment) == NULL) {
				if (READ_ONCE(e->rbias))
					continue;
				(void)xchg(slot, NULL);
			}
		}
#endif

		for (;;) {
			trials++;

			smp_mb__before_atomic();
			old = atomic_add_unless(&e->segment, 1, wlock);
			smp_mb__after_atomic();

			if (old != 0)
				break;
			if ((trials % MAX_TRY_COUNT) == 0) {
				if (trials == MAX_RETRIES_COUNT)
					break;
			}
		}

#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
		if (((this_cpu_inc_return(check_bias) % CHECK_FOR_BIAS) == 0) &&
		    (!READ_ONCE(e->rbias) && rdtsc() >= e->inhibit_until))
			WRITE_ONCE(e->rbias, 1);
#endif
	}
}

void range_read_unlock(struct range_lock *lock, unsigned long start, unsigned long size)
{
	unsigned long i;
	unsigned long end = start + size;
	struct sg_entry *e;
#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
	u32 id;
	volatile atomic_t **slot = NULL;
#endif

	for (i = start; i < end; ++i) {
		e = &lock->sg_table[i];
#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
#if defined(RGLOCK_BRAVO)
		id = hash((uint64_t)(&e->segment));
		slot = &lock->vr_table[V(id)];
#elif defined(GLOBAL_RGLOCK_BRAVO)
		id = hash((uint64_t)(&e->segment));
		slot = &global_vr_table[V(id)];
#endif
		if (cmpxchg(slot, &e->segment, NULL) == &e->segment)
			continue;
#endif
		smp_mb__before_atomic();
		atomic_dec(&e->segment);
		smp_mb__after_atomic();
	}
}

static inline void wait_for_visible_reader(struct range_lock *lock,
					   struct sg_entry *e)
{
#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
	if (READ_ONCE(e->rbias)) {
		int i, j;
		unsigned long start, now;
		volatile atomic_t **vr_table;

		smp_mb();
		WRITE_ONCE(e->rbias, 0);


#if defined(RGLOCK_BRAVO)
		vr_table = lock->vr_table;
#elif defined(GLOBAL_RGLOCK_BRAVO)
		vr_table = global_vr_table;
#endif
		start = rdtsc();
		for (i = 0; i < TABLE_SIZE; i += 8) {
			uint64_t value = (uint64_t)vr_table[V(i)] |
				(uint64_t)vr_table[V(i + 1)] |
				(uint64_t)vr_table[V(i + 2)] |
				(uint64_t)vr_table[V(i + 3)] |
				(uint64_t)vr_table[V(i + 4)] |
				(uint64_t)vr_table[V(i + 5)] |
				(uint64_t)vr_table[V(i + 6)] |
				(uint64_t)vr_table[V(i + 7)];
			if ((uint64_t)value != 0) {
				for (j = 0; j < 8; j++) {
					while ((uint64_t)vr_table[V(i + j)] ==
							(uint64_t)&(e->segment)) {
						if (need_resched())
							schedule();
					}
				}
			}
		}
		now = rdtsc();
		e->inhibit_until = now + ((now - start) * N);
	}
#endif
}

void range_write_lock(struct range_lock *lock, unsigned long start, unsigned long size)
{
	unsigned long trials;
	unsigned int wlock = (unsigned int)1 << 31;
	unsigned long i, end = start + size;
	int old;
	struct sg_entry *e;

#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
	u32 id;
	volatile atomic_t *slot = NULL;
#endif

	for (i = start; i < end; ++i) {
		trials = 0;
		e = &lock->sg_table[i];

		for (;;) {
			trials++;

			smp_mb__before_atomic();
			old = atomic_cmpxchg(&e->segment, 0, wlock);
			smp_mb__after_atomic();

			if (old == 0)
				break;

			if ((trials % MAX_TRY_COUNT) == 0) {
				if (trials == MAX_RETRIES_COUNT)
					break;
			}


			if (need_resched())
				schedule();
		}

#if BRAVO_NO_REVOCATION
#if defined(RGLOCK_BRAVO)
		id = hash((uint64_t)(&e->segment));
		slot = lock->vr_table[V(id)];
#elif defined(GLOBAL_RGLOCK_BRAVO)
		id = hash((uint64_t)(&e->segment));
		slot = global_vr_table[V(id)];
#endif
		while (slot == &(e->segment)) {
			if (need_resched())
				schedule();
		}
#else
		wait_for_visible_reader(lock, e);
#endif
	}
}

void range_write_unlock(struct range_lock *lock, unsigned long start, unsigned long size)
{
	unsigned long trials;
	unsigned int wlock = (unsigned int)1 << 31;
	unsigned long i, end = start + size;
	int old;
	struct sg_entry *e;

	for (i = start; i < end; ++i) {
		e = &lock->sg_table[i];
		trials = 0;

		for (;;) {
			trials++;

			smp_mb__before_atomic();
			old = atomic_cmpxchg(&e->segment, wlock, 0);
			smp_mb__after_atomic();

			if (old == wlock)
				break;

			if ((trials % MAX_TRY_COUNT) == 0) {
				if (trials == MAX_RETRIES_COUNT)
					break;
			}
		}
	}
}


void range_write_upgrade(struct range_lock *lock, unsigned long start,
			 unsigned long size)
{
	/* Hold the lock from the beginning until the end */	
	if (start != 0)
		range_write_lock(lock, 0, start - 1);
}

void range_write_downgrade(struct range_lock *lock, unsigned long start,
			   unsigned long size)
{
	/* Release the lock from the beginning upto the start */
	if (start != 0)
		range_write_unlock(lock, 0, start - 1);
}

#elif defined(RANGE_LOCK_LIST)

#include <linux/smp.h>
#include <linux/bug.h>
#include <linux/percpu.h>
#include <linux/hardirq.h>
#include <linux/prefetch.h>
#include <linux/atomic.h>
#include <asm/byteorder.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/slab.h>


static int compare(volatile struct range_lock_node *node1,
		   volatile struct range_lock_node *node2)
{
	int readers;

	if (!node1)
		return 1;
	readers = (int)node1->reader + (int)node2->reader;
	if (node1->start >= node2->end)
		return 1;
	if (node1->start >= node2->start && readers == 2)
		return 1;
	if (node2->start >= node1->end)
		return -1;
	if (node2->start >= node1->start && readers == 2)
		return -1;
	return 0;
}

static __always_inline bool marked(volatile struct range_lock_node *node)
{
	return (((u64)node % 2) == 1);
}

static __always_inline struct range_lock_node *
unmark(volatile struct range_lock_node *node)
{
	return (struct range_lock_node *)((u64)node - 1);
}

static inline bool validate_reader(volatile struct range_lock *lock,
				   volatile struct range_lock_node *node)
{
	volatile struct range_lock_node **prev;
	volatile struct range_lock_node *cur, *next;
	int ret;

	prev = (volatile struct range_lock_node **)&node;
	cur = node;
	while (true) {
		if (cur && marked(cur->next)) {
			next = unmark(cur->next);
			if (cmpxchg(prev, cur, next) == cur) {
				rcu_read_unlock();
				kfree_rcu(cur);
				rcu_read_lock();
			}
			cur = next;
		} else {
			if (cur == node) {
				prev = (volatile struct range_lock_node **)&(
					cur->next);
				cur = *prev;
				continue;
			}
			ret = compare(cur, node);
			if (ret == 0) {
				while (!marked(cur->next))
					cpu_relax();
			} else
				return true;
		}
	}
	BUG(); //During traversal, either it should found a node which does not overlap or should have reached the end of the list.
	return true;
}

static inline bool validate_writer(volatile struct range_lock *lock,
				   volatile struct range_lock_node *node)
{
	volatile struct range_lock_node **prev;
	volatile struct range_lock_node *cur, *next;
	int ret;

	while (true) {
		prev = (volatile struct range_lock_node **)&(lock->head);
		cur = *prev;
		while (true) {
			if (marked(cur))
				break;
			else if (cur && cur == node)
				return true;
			else if (cur && marked(cur->next)) {
				next = unmark(cur->next);
				if (cmpxchg(prev, cur, next) == cur) {
					rcu_read_unlock();
					kfree_rcu(cur);
					rcu_read_lock();
				}
				cur = next;
			} else {
				ret = compare(cur, node);
				if (ret == -1) {
					prev = (volatile struct range_lock_node **)&(
						cur->next);
					cur = *prev;
				} else if (ret == 0)
					return false;
				else if (ret == 1) {
					BUG(); //Writer will finds its own node first before this condition is satisfied.
					return false;
				}
			}
		}
	}
}

static bool insert_rl_node(volatile struct range_lock *lock,
			   volatile struct range_lock_node *node)
{
	volatile struct range_lock_node **prev;
	volatile struct range_lock_node *cur, *next;
	int ret;
	bool ret_val;

	rcu_read_lock();
	while (true) {
		prev = (volatile struct range_lock_node **)&(lock->head);
		cur = *prev;
		while (true) {
			if (marked(cur))
				break;
			else if (cur && marked(cur->next)) {
				next = unmark(cur->next);
				if (cmpxchg(prev, cur, next) == cur) {
					rcu_read_unlock();
					kfree_rcu(cur);
					rcu_read_lock();
				}
				cur = next;
			} else {
				ret = compare(cur, node);
				if (ret == -1) {
					prev = (volatile struct range_lock_node **)&(
						cur->next);
					cur = *prev;
				} else if (ret == 0) {
					while (!marked(cur->next))
						cpu_relax();
				} else if (ret == 1) {
					node->next = cur;
					if (cmpxchg(prev, cur, node) == cur) {
						if (node->reader) {
							ret_val =
								validate_reader(
									lock,
									node);
							rcu_read_unlock();
							return ret_val;
						} else {
							ret_val =
								validate_writer(
									lock,
									node);
							rcu_read_unlock();
							return ret_val;
						}
					}
					cur = *prev;
				}
			}
		}
	}
	rcu_read_unlock();
}

static inline void fetch_and_add(volatile struct range_lock_node **variable,
				 int value)
{
	__asm__ volatile("lock; xaddl %0, %1"
			 : "+r"(value), "+m"(*variable) // input + output
			 : // No input-only
			 : "memory");
}

__always_inline void rw_range_unlock(volatile struct range_lock_node *node)
{
	fetch_and_add((volatile struct range_lock_node **)&(node->next), 1);
}

__always_inline volatile struct range_lock_node *
rw_range_lock(volatile struct range_lock *lock, long start, long end, bool reader)
{
	volatile struct range_lock_node *rl_node = NULL;

	do {
		if (rl_node)
			rw_range_unlock(rl_node);
		rl_node = (struct range_lock_node *)kmalloc(
			sizeof(struct range_lock_node), GFP_KERNEL);
		rl_node->start = start;
		rl_node->end = end;
		rl_node->next = NULL;
		rl_node->reader = reader;
	} while (!insert_rl_node(lock, rl_node));

	return rl_node;
}

__always_inline void *
range_read_lock(volatile struct range_lock *lock,
		unsigned long start, unsigned long end)
{
	return (void *)rw_range_lock(lock, start, end, true);
}

__always_inline void range_read_unlock(void *node)
{
	rw_range_unlock((volatile struct range_lock_node *)node);
}

__always_inline void *
range_write_lock(volatile struct range_lock *lock,
		 unsigned long start, unsigned long end)
{
	return (void *)rw_range_lock(lock, start, end, false);
}

__always_inline void range_write_unlock(void *node)
{
	rw_range_unlock((volatile struct range_lock_node *)node);
}

#endif
