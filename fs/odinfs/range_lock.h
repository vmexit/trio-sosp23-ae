#ifndef __SEGMENT_RANGE_LOCK_H__
#define __SEGMENT_RANGE_LOCK_H__

#include <linux/percpu.h>
#include <linux/vmalloc.h>

#ifdef RANGE_LOCK_SEGMENT

//#define SEGMENT_INIT_COUNT 	(2048) /* 1024 * 1024 * 8 */

/* INIT file size: 8MB */
#define SEGMENT_INIT_COUNT 	(2048)
#define SEGMENT_SIZE_BITS 	12

#define FILE_NODE_SEGMENT_RANGE 9 /* a single node covers 512 segments */
#define FILE_NODE_SEGMENT_BITS 	13

/* Ad-hoc code to work with */
#define MAX_TRY_COUNT		100000
#define MAX_RETRIES_COUNT 	1000000000

#define GLOBAL_RGLOCK_BRAVO 1

//#define RGLOCK_BRAVO 1
//#define CL_ALIGNED_TABLE

#define BRAVO_NO_REVOCATION 1

#ifdef RGLOCK_BRAVO

#ifndef CL_ALIGNED_TABLE
#define NUM_SLOT        4096
#define TABLE_SIZE      (NUM_SLOT)
#define V(i)            (i)
#else
#define NUM_SLOT        4096
#define TABLE_SIZE      (L1_CACHE_BYTES * (NUM_SLOT))
#define V(i)            ((i) * L1_CACHE_BYTES)
#endif
#define N               	9
#define CHECK_FOR_BIAS  	16

#endif

#ifdef GLOBAL_RGLOCK_BRAVO
#define NUM_SLOT	(1024*1024)
#define TABLE_SIZE	(NUM_SLOT)
#define V(i)		((i))

#define N			9
#define CHECK_FOR_BIAS		16

extern volatile atomic_t **global_vr_table;

#define pmfs_init_global_rglock_bravo(l) {				\
	l = vzalloc(TABLE_SIZE * sizeof(atomic_t *));			\
}

#define pmfs_free_global_rglock_bravo(l) {				\
	vfree(l);							\
}
#endif

extern struct kmem_cache *pmfs_rangelock_cachep;

struct range_lock *range_lock_init(unsigned long size);
void range_lock_free(struct range_lock *lock);

void range_read_lock(struct range_lock *lock, unsigned long start, unsigned long size);
void range_read_unlock(struct range_lock *lock, unsigned long start, unsigned long size);
void range_write_lock(struct range_lock *lock, unsigned long start, unsigned long size);
void range_write_unlock(struct range_lock *lock, unsigned long start, unsigned long size);
void range_write_upgrade(struct range_lock *lock, unsigned long start, unsigned long size);
void range_write_downgrade(struct range_lock *lock, unsigned long start, unsigned long size);

struct sg_entry {
	atomic_t segment;
#if defined(RGLOCK_BRAVO) || defined(GLOBAL_RGLOCK_BRAVO)
	volatile int rbias;
	volatile u64 inhibit_until;
#endif
};

struct range_lock {
	struct sg_entry *sg_table;
	long sg_size;
#ifdef RGLOCK_BRAVO
	volatile atomic_t **vr_table;	
#endif
};

#ifdef PMFS_ENABLE_RANGE_LOCK_KMEM_CACHE
#ifndef RGLOCK_BRAVO
#define pmfs_init_range_lock(l) {						\
	(l)->sg_table = kmem_cache_alloc(pmfs_rangelock_cachep, GFP_KERNEL); 	\
	(l)->sg_size = SEGMENT_INIT_COUNT; \
}
#else
#define pmfs_init_range_lock(l) {						\
	(l)->sg_table = kmem_cache_alloc(pmfs_rangelock_cachep, GFP_KERNEL); 	\
	(l)->vr_table = vzalloc(TABLE_SIZE * sizeof(atomic_t *)); 		\
	(l)->sg_size = SEGMENT_INIT_COUNT; \
}
#endif
#else
#ifndef RGLOCK_BRAVO
#define pmfs_init_range_lock(l) {						\
	(l)->sg_table = vzalloc(SEGMENT_INIT_COUNT * sizeof(struct sg_entry)); 	\
	(l)->sg_size = SEGMENT_INIT_COUNT; \
}
#else
#define pmfs_init_range_lock(l) {						\
	(l)->sg_table = vzalloc(SEGMENT_INIT_COUNT * sizeof(struct sg_entry)); 	\
	(l)->vr_table = vzalloc(TABLE_SIZE * sizeof(atomic_t *)); 		\
	(l)->sg_size = SEGMENT_INIT_COUNT; \
}
#endif
#endif

#define allocate_rlock_info() unsigned long start_seg, end_seg;

#define call_xip_file_read() { 					\
	res = xip_file_read(filp, buf, len, ppos); 		\
}

/* Make sure the end_seg does not exceed isize during read */
#define assign_rlock_info_read(isize) {					\
	start_seg = pos >> SEGMENT_SIZE_BITS; 			\
	if (pos + len <= isize) \
		end_seg = (pos + len - 1) >> SEGMENT_SIZE_BITS; 	\
	else	 \
		end_seg = (isize - 1) >> SEGMENT_SIZE_BITS; \
}

/*
 * Do not need the check for write since if pos + len - 1 > isize
 * (i.e., appending), we won't acquire the range lock
 */
#define assign_rlock_info_write() {                   \
    start_seg = pos >> SEGMENT_SIZE_BITS;           \
    end_seg = (pos + len - 1) >> SEGMENT_SIZE_BITS;     \
}


#define get_rangelock_info(i) 	(&((i)->rlock))
#define get_range_lock(i) get_rangelock_info(PMFS_I((i)))
#define get_start_range() (start_seg)
#define get_size_or_end_range() (end_seg - start_seg + 1)

#define RANGE_READ_LOCK(l, s, e) range_read_lock((l), (s), (e))
#define RANGE_READ_UNLOCK(l, s, e) range_read_unlock((l), (s), (e))
#define RANGE_WRITE_LOCK(l, s, e) range_write_lock((l), (s), (e))
#define RANGE_WRITE_UNLOCK(l, s, e) range_write_unlock((l), (s), (e))

static inline void pmfs_resize_range_lock(struct range_lock *l, long new_size) {

	long old_sg_size;
	int need_realloc = 0;
	
	old_sg_size = l->sg_size;

	while (new_size >> SEGMENT_SIZE_BITS >= l->sg_size) {
		l->sg_size *= 2;
		need_realloc = 1;
	}

	if (need_realloc) {

#ifdef PMFS_ENABLE_RANGE_LOCK_KMEM_CACHE
	if(old_sg_size == SEGMENT_INIT_COUNT)
	       kmem_cache_free(pmfs_rangelock_cachep, l->sg_table);
	else
		vfree(l->sg_table);
#else	
	vfree(l->sg_table);
#endif
		l->sg_table= vzalloc(l->sg_size * sizeof(struct sg_entry));
	}
}

#elif defined(RANGE_LOCK_LIST)

struct range_lock_node {
	u64 start;
	u64 end;
	int reader;
	volatile struct range_lock_node *next;
};

struct range_lock {
	volatile struct range_lock_node *head;
};


extern volatile struct range_lock_node *read_range_lock(volatile struct range_lock *lock,
						   long start, long end);

extern void read_range_unlock(volatile struct range_lock_node *node);

extern volatile struct range_lock_node *
write_range_lock(volatile struct range_lock *lock, long start, long end);
extern void write_range_unlock(volatile struct range_lock_node *node);

#define pmfs_init_range_lock(l) {						\
	(l)->head = NULL;							\
}


#define allocate_rlock_info() unsigned long start_seg, end_seg; void *node;

#define call_xip_file_read() { 					\
	res = xip_file_read(filp, buf, len, ppos); 		\
}

#define assign_rlock_info() {					\
	start_seg = pos; 					\
	end_seg = (pos + len - 1); 				\
}

#define get_rangelock_info(i) 	(&((i)->rlock))
#define get_range_lock(i) get_rangelock_info(PMFS_I((i)))
#define get_start_range() (start_seg)
#define get_size_or_end_range() (end_seg)

#define RANGE_READ_LOCK(l, s, e) {					\
	node = range_read_lock((volatile struct range_lock *)(l),	\
		       		(s), (e));\
}

#define RANGE_READ_UNLOCK(l, s, e) range_read_unlock((node));

#define RANGE_WRITE_LOCK(l, s, e) {					\
	node = range_write_lock((volatile struct range_lock *)(l),	\
				(s), (e));\
}

#define RANGE_WRITE_UNLOCK(l, s, e) range_write_unlock((node));

#define range_lock_free(l) {}

extern void *range_read_lock(volatile struct range_lock *lock,
		      unsigned long start, unsigned long end);
extern void range_read_unlock(void *node);
extern void *range_write_lock(volatile struct range_lock *lock,
		       unsigned long start, unsigned long size);
extern void range_write_unlock(void *node);

#endif

#endif /* __SEGMENT_RANGE_LOCK_H__ */
