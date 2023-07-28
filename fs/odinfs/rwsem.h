#ifndef __RWSEM_H_
#define __RWSEM_H_

#include <linux/percpu-rwsem.h>

#include "pmfs_config.h"

#include "pcpu_rwsem.h"


#define pmfs_pinode_rwsem(i) (&((i)->rwlock))

#define pmfs_inode_rwsem(i) (pmfs_pinode_rwsem(PMFS_I((i))))

#if PMFS_INODE_LOCK == PMFS_INODE_LOCK_STOCK
#define pmfs_inode_rwsem_down_read(i) (inode_lock_shared(i))
#elif PMFS_INODE_LOCK == PMFS_INODE_LOCK_MAX_PERCPU
#define pmfs_inode_rwsem_down_read(i) (pcpu_rwsem_down_read(pmfs_inode_rwsem(i)))
#elif  PMFS_INODE_LOCK == PMFS_INODE_LOCK_PERCPU
#define pmfs_inode_rwsem_down_read(i) (percpu_down_read(pmfs_inode_rwsem(i)))
#endif


#if PMFS_INODE_LOCK == PMFS_INODE_LOCK_STOCK
#define pmfs_inode_rwsem_up_read(i) (inode_unlock_shared(i))
#elif PMFS_INODE_LOCK == PMFS_INODE_LOCK_MAX_PERCPU
#define pmfs_inode_rwsem_up_read(i) (pcpu_rwsem_up_read(pmfs_inode_rwsem(i)))
#elif  PMFS_INODE_LOCK == PMFS_INODE_LOCK_PERCPU
#define pmfs_inode_rwsem_up_read(i) (percpu_up_read(pmfs_inode_rwsem(i)))
#endif

#if PMFS_INODE_LOCK == PMFS_INODE_LOCK_STOCK
#define pmfs_inode_rwsem_down_write(i) (inode_lock(i))
#elif PMFS_INODE_LOCK == PMFS_INODE_LOCK_MAX_PERCPU
#define pmfs_inode_rwsem_down_write(i) (pcpu_rwsem_down_write(pmfs_inode_rwsem(i)))
#elif  PMFS_INODE_LOCK == PMFS_INODE_LOCK_PERCPU
#define pmfs_inode_rwsem_down_write(i) (percpu_down_write(pmfs_inode_rwsem(i)))
#endif

#if PMFS_INODE_LOCK == PMFS_INODE_LOCK_STOCK
#define pmfs_inode_rwsem_up_write(i) (inode_unlock(i))
#elif PMFS_INODE_LOCK == PMFS_INODE_LOCK_MAX_PERCPU
#define pmfs_inode_rwsem_up_write(i) (pcpu_rwsem_up_write(pmfs_inode_rwsem(i)))
#elif  PMFS_INODE_LOCK == PMFS_INODE_LOCK_PERCPU
#define pmfs_inode_rwsem_up_write(i) (percpu_up_write(pmfs_inode_rwsem(i)))
#endif


#if PMFS_INODE_LOCK == PMFS_INODE_LOCK_STOCK
#define pmfs_pinode_rwsem_init(i)
#elif PMFS_INODE_LOCK == PMFS_INODE_LOCK_MAX_PERCPU
#define pmfs_pinode_rwsem_init(i) (pcpu_rwsem_init_rwsem(pmfs_pinode_rwsem(i)))
#elif  PMFS_INODE_LOCK == PMFS_INODE_LOCK_PERCPU
#define pmfs_pinode_rwsem_init(i) (percpu_init_rwsem(pmfs_pinode_rwsem(i)))
#endif

#if PMFS_INODE_LOCK == PMFS_INODE_LOCK_STOCK
#define pmfs_pinode_rwsem_free(i)
#elif PMFS_INODE_LOCK == PMFS_INODE_LOCK_MAX_PERCPU
#define pmfs_pinode_rwsem_free(i) (pcpu_rwsem_free_rwsem(pmfs_pinode_rwsem(i)))
#elif  PMFS_INODE_LOCK == PMFS_INODE_LOCK_PERCPU
//#define pmfs_pinode_rwsem_free(i) (percpu_free_rwsem(pmfs_pinode_rwsem(i)))
#define pmfs_pinode_rwsem_free(i)
#endif




#endif /* __RWSEM_H_ */
