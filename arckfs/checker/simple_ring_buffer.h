#ifndef SUFS_CHECKER_SIMPLE_RING_BUFFER_H_
#define SUFS_CHECKER_SIMPLE_RING_BUFFER_H_

#include "../include/checker_config.h"
#include "../include/ring_buffer.h"
#include "../lib/spinlock.h"

static inline struct sufs_ring_buffer * 
sufs_checker_ring_buffer_connect(unsigned long addr)
{
    struct sufs_ring_buffer *ret = (struct sufs_ring_buffer *) (addr);

    ret->libfs_requests = (struct sufs_ring_buffer_entry *)
            (((unsigned long) ret) + sizeof(struct sufs_ring_buffer));

#if 0
    printf("size of struct ring_buffer_entry: %ld\n",
            sizeof(struct sufs_ring_buffer));

    printf("libfs_requests addr: %lx\n", (unsigned long) (ret->libfs_requests));
#endif

    return ret;
}


static inline 
int sufs_checker_do_sr_send_request(struct sufs_ring_buffer *ring, void *from)
{
    int ret = 0;
    int my_idx = 0;

    sufs_spin_lock(&ring->spinlock);

    my_idx = ring->producer_idx;

    if (ring->libfs_requests[my_idx].valid)
    {
#if 0
        printf("Ring buffer full!\n");
#endif
        ret = -SUFS_RBUFFER_AGAIN;
        sufs_spin_unlock(&ring->spinlock);
        return ret;
    }

    ring->producer_idx = (ring->producer_idx + 1) % (ring->num_of_entry);

    sufs_spin_unlock(&ring->spinlock);

#if 0
    printf("request is %lx, ring->entry_size is %ld\n",
            (unsigned long) &(ring->libfs_requests[my_idx].request), ring->entry_size);
#endif

    memcpy(&(ring->libfs_requests[my_idx].request), from, ring->entry_size);

    ring->libfs_requests[my_idx].valid = 1;

    return 0;

}

static inline void 
sufs_checker_sr_send_request(struct sufs_ring_buffer *ring, void *from)
{
    int ret = 0; 
    do
    {
        ret = sufs_checker_do_sr_send_request(ring, from);
    } while (ret == -SUFS_RBUFFER_AGAIN);
}

static inline int
sufs_checker_do_sr_receive_request(struct sufs_ring_buffer *ring, void *to)
{
    int ret = 0;
    int my_idx = 0;

    sufs_spin_lock(&ring->spinlock);

    my_idx = ring->comsumer_idx;

    if (!ring->libfs_requests[my_idx].valid)
    {
#if 0
        printf("Ring buffer full!\n");
#endif
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
sufs_checker_sr_receive_request(struct sufs_ring_buffer *ring, void *to)
{
    int ret = 0;
    do
    {
        ret = sufs_checker_do_sr_receive_request(ring, to);
    } while (ret == -SUFS_RBUFFER_AGAIN);
}

#endif
