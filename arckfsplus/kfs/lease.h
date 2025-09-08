#ifndef SUFS_KFS_LEASE_H_
#define SUFS_KFS_LEASE_H_

#include <linux/spinlock.h>
#include <linux/sched.h>

#include "../include/kfs_config.h"
#include "../include/priv_inode.h"
#include "../include/ring_buffer.h"
#include "../include/ioctl.h"
#include "../lib/spinlock.h"
#include "simple_ring_buffer.h"

extern struct sufs_kfs_lease sufs_kfs_rename_lease; 

int sufs_kfs_acquire_write_lease(int ino, struct sufs_kfs_lease * l, 
    struct sufs_shadow_inode * sinode, int tgid);

int sufs_kfs_acquire_read_lease(int ino, struct sufs_kfs_lease * l, 
    struct sufs_shadow_inode * sinode, int tgid);

int sufs_kfs_release_lease(int ino, struct sufs_kfs_lease * l, int tgid);

int sufs_kfs_renew_lease(int ino);

static inline void
sufs_kfs_init_lease(struct sufs_kfs_lease * l)
{
    memset(l, 0, sizeof(struct sufs_kfs_lease));
    sufs_spin_init(&(l->lock));
}

static inline void 
sufs_lease_queue_send(struct sufs_ring_buffer *q, int ino_num)
{
    int ret = 0;
    struct sufs_ioctl_to_free_entry e;
    e.ino_num = ino_num; 
    do
    {
        ret = sufs_kfs_sr_send_request(q, &e);
    } while (ret == -SUFS_RBUFFER_AGAIN);
}

static inline void 
sufs_kfs_rename_lease_init(void)
{
    sufs_kfs_init_lease(&sufs_kfs_rename_lease);
}

int sufs_kfs_acquire_rename_lease(struct sufs_kfs_lease *l);

int sufs_kfs_release_rename_lease(struct sufs_kfs_lease *l);

#endif /* SUFS_KFS_LEASE_H_ */
