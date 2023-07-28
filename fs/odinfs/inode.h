#ifndef __ODINFS_INODE_H__
#define __ODINFS_INODE_H__

#include <linux/fs.h>


#include "pcpu_rwsem.h"
#include "pmfs_config.h"

struct pmfs_inode_info {
	__u32 i_dir_start_lookup;
	struct list_head i_truncated;
	struct pmfs_inode_info_header header;
	struct inode vfs_inode;

	struct range_lock rlock;
    /* Need cacheline break here? */
#if PMFS_INODE_LOCK == PMFS_INODE_LOCK_MAX_PERCPU
    struct pcpu_rwsem rwlock;
#elif PMFS_INODE_LOCK == PMFS_INODE_LOCK_PERCPU
    struct percpu_rw_semaphore rwlock;
#endif
};

static inline struct pmfs_inode_info *PMFS_I(struct inode *inode)
{
	return container_of(inode, struct pmfs_inode_info, vfs_inode);
}

extern void pmfs_update_isize(struct inode *inode, struct pmfs_inode *pi);
extern void pmfs_update_nlink(struct inode *inode, struct pmfs_inode *pi);
extern void pmfs_update_time(struct inode *inode, struct pmfs_inode *pi);

extern void pmfs_set_inode_flags(struct inode *inode, struct pmfs_inode *pi);
extern void pmfs_get_inode_flags(struct inode *inode, struct pmfs_inode *pi);
#endif 
