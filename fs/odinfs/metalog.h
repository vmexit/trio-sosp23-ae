#ifndef __PHASE_META_LOG_H__
#define __PHASE_META_LOG_H__

#include <linux/spinlock.h>
#include <linux/smp.h>
#include <linux/threads.h>
#include <linux/percpu.h>
#include <linux/seqlock.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/gfp.h>

#include "pcpu_rwsem.h"

/* Handling the case for size */

#define READ_SIZE_OP 	1
#define INCR_SIZE_OP 	2
#define DECR_SIZE_OP 	4

struct phase_meta_info {
	raw_spinlock_t 	lock;
	ssize_t 	val;
	ssize_t __percpu *vals;

	int 		bias;
	atomic_t 	phase;
	u64 		inhibit_until;
	struct pcpu_rwsem phase_lock;
};

#define phase_read_lock(l) 	pcpu_rwsem_down_read(l)
#define phase_read_unlock(l) 	pcpu_rwsem_up_read(l)
#define phase_write_lock(l) 	pcpu_rwsem_down_write(l)
#define phase_write_unlock(l) 	pcpu_rwsem_up_write(l)
#define phase_change_lock(l) 	pcpu_rwsem_down_write(l)
#define phase_change_unlock(l)	pcpu_rwsem_up_write(l)

void inode_size_phase_info_init(struct phase_meta_info *pinfo);
void inode_size_phase_info_destroy(struct phase_meta_info *pinfo);
ssize_t inode_size_read(struct phase_meta_info *pinfo);
void inode_size_change_phase(struct phase_meta_info *pinfo, int phase);
void inode_size_inc(struct phase_meta_info *pinfo, ssize_t value);
void inode_size_dec(struct phase_meta_info *pinfo, ssize_t value);
void inode_size_sync(struct phase_meta_info *pinfo);
void inode_size_reset(struct phase_meta_info *pinfo);
ssize_t inode_size_sync_read(struct phase_meta_info *pinfo);

#endif /* __PHASE_META_LOG_H__ */
