#ifndef SUFS_LIBFS_SIMPLE_RING_BUFFER_H_
#define SUFS_LIBFS_SIMPLE_RING_BUFFER_H_

#include "../../include/libfs_config.h"
#include "../../include/ring_buffer.h"
#include "../../lib/spinlock.h"
#include "super.h"

extern struct sufs_ring_buffer * sufs_libfs_ring_buffers[SUFS_PM_MAX_INS][SUFS_MAX_CPU / SUFS_PM_MAX_INS];

extern struct sufs_ring_buffer * sufs_libfs_lease_ring;

int sufs_libfs_sr_send_request(struct sufs_ring_buffer *ring, void *from);

void sufs_libfs_ring_buffer_connect(struct sufs_libfs_super_block * sb);

static inline int sufs_libfs_sr_is_empty(struct sufs_ring_buffer *ring)
{
    return (!ring->libfs_requests[ring->comsumer_idx].valid);
}

static inline int 
sufs_libfs_do_sr_receive_request(struct sufs_ring_buffer *ring, void *to)
{
    int ret = 0;
    int my_idx = 0;

    sufs_spin_lock(&ring->spinlock);

    my_idx = ring->comsumer_idx;

    if (!ring->libfs_requests[my_idx].valid)
    {
        ret = -SUFS_RBUFFER_AGAIN;
        sufs_spin_unlock(&ring->spinlock);
        return ret;
    }

    ring->comsumer_idx = (ring->comsumer_idx + 1) % ring->num_of_entry;
    sufs_spin_unlock(&ring->spinlock);

    memcpy(to, &(ring->libfs_requests[my_idx].request),
           ring->entry_size);

    ring->libfs_requests[my_idx].valid = 0;    
    return 0;
}

static inline void
sufs_libfs_sr_receive_request(struct sufs_ring_buffer *ring, void *to)
{
    int ret = 0;
    do
    {
        ret = sufs_libfs_do_sr_receive_request(ring, to);
    } while (ret == -SUFS_RBUFFER_AGAIN);
}


#endif
