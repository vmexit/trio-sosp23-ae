#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../include/libfs_config.h"
#include "../include/ring_buffer.h"
#include "../lib/spinlock.h"

#include "super.h"

struct sufs_ring_buffer * sufs_libfs_lease_ring;

struct sufs_ring_buffer * sufs_libfs_ring_buffers[SUFS_PM_MAX_INS][SUFS_MAX_CPU / SUFS_PM_MAX_INS];


static struct sufs_ring_buffer * sufs_libfs_sr_create_connect(int index)
{
    int size = SUFS_ODIN_ONE_RING_SIZE;
    struct sufs_ring_buffer *ret = NULL;

    ret = (struct sufs_ring_buffer *) ((SUFS_ODIN_RING_ADDR) + index * size);

    ret->libfs_requests = (struct sufs_ring_buffer_entry *)
            (((unsigned long) ret) + sizeof(struct sufs_ring_buffer));
        
    return ret;
}

void sufs_libfs_ring_buffer_connect(struct sufs_libfs_super_block * sb)
{
    int i = 0, j = 0;

    int num_of_rings_per_socket = sb->dele_ring_per_node;

    sufs_libfs_lease_ring = (struct sufs_ring_buffer *) (SUFS_LEASE_RING_ADDR);

    sufs_libfs_lease_ring->libfs_requests = (struct sufs_ring_buffer_entry *)
            (((unsigned long) sufs_libfs_lease_ring) + 
                sizeof(struct sufs_ring_buffer));

    for (i = 0; i < sb->pm_nodes; i++)
    {
        for (j = 0; j < sb->dele_ring_per_node; j++)
        {
            sufs_libfs_ring_buffers[i][j] =
                    sufs_libfs_sr_create_connect(i * num_of_rings_per_socket + j);
        }
    }
}


int sufs_libfs_sr_send_request(struct sufs_ring_buffer *ring, void *from)
{
    int ret = 0;
    int my_idx = 0;

    sufs_spin_lock(&ring->spinlock);

    my_idx = ring->producer_idx;

    if (ring->libfs_requests[my_idx].valid)
    {
        ret = -SUFS_RBUFFER_AGAIN;
        sufs_spin_unlock(&ring->spinlock);
        return ret;
    }

    ring->producer_idx = (ring->producer_idx + 1) % (ring->num_of_entry);

    sufs_spin_unlock(&ring->spinlock);

    memcpy(&(ring->libfs_requests[my_idx].request), from, ring->entry_size);

    ring->libfs_requests[my_idx].valid = 1;

    return 0;
}






