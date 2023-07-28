#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#include "delegation.h"
#include "pmfs_config.h"
#include "pmfs_stats.h"
#include "ring.h"
#include "simple_ring.h"
#include "pmfs.h"
#include "sufs_spinlock.h"

struct spmfs_ring_buffer *pmfs_sr_create(int socket, int entry_size, int size)
{
	struct spmfs_ring_buffer *ret;

	ret = kmalloc_node(sizeof(struct spmfs_ring_buffer), GFP_KERNEL,
			   socket);

	if (ret == NULL)
		return NULL;

	/* kcalloc cannot allocate more than 4MB size in our kernel */
	ret->requests = vzalloc_node(size, socket);

	if (ret->requests == NULL) {
		kfree(ret);
		return NULL;
	}

	ret->comsumer_idx = 0;
	ret->producer_idx = 0;
	ret->entry_size = entry_size;
	/* See comments in simple_ring.h */
	ret->num_of_entry = size / (sizeof(struct spmfs_ring_buffer_entry));

#if PMFS_RING_SIMPLE_LOCK
	sufs_spin_init(&ret->spinlock);
#else
	spin_lock_init(&ret->spinlock);
#endif

	return ret;
}

void pmfs_sr_destroy(struct spmfs_ring_buffer *ring)
{
	if (ring->requests)
		vfree(ring->requests);

	kfree(ring);
}

int pmfs_sr_send_request(struct spmfs_ring_buffer *ring, void *from)
{
	unsigned long irq;
	int ret = 0;

#if PMFS_RING_SIMPLE_LOCK
	local_irq_save(irq);
	sufs_spin_lock(&ring->spinlock);
#else
	spin_lock_irqsave(&ring->spinlock, irq);
#endif

	if (ring->requests[ring->producer_idx].valid) {
		ret = -EAGAIN;
		goto out;
	}

	memcpy(&(ring->requests[ring->producer_idx].request), from,
	       ring->entry_size);

	ring->requests[ring->producer_idx].valid = 1;

	ring->producer_idx = (ring->producer_idx + 1) % (ring->num_of_entry);

out:
#if PMFS_RING_SIMPLE_LOCK
    sufs_spin_unlock(&ring->spinlock);
    local_irq_restore(irq);
#else
	spin_unlock_irqrestore(&ring->spinlock, irq);
#endif
	return ret;
}

int pmfs_sr_receive_request(struct spmfs_ring_buffer *ring, void *to)
{
	unsigned long irq;
	int ret = 0;

	/* Spin to wait for a new entry */
#if PMFS_RING_SIMPLE_LOCK
    local_irq_save(irq);
    sufs_spin_lock(&ring->spinlock);
#else
	spin_lock_irqsave(&ring->spinlock, irq);
#endif

	if (!ring->requests[ring->comsumer_idx].valid) {
		ret = -EAGAIN;
		goto out;
	}

	memcpy(to, &(ring->requests[ring->comsumer_idx].request),
	       ring->entry_size);

	ring->requests[ring->comsumer_idx].valid = 0;

	ring->comsumer_idx = (ring->comsumer_idx + 1) % ring->num_of_entry;

out:
#if PMFS_RING_SIMPLE_LOCK
    sufs_spin_unlock(&ring->spinlock);
    local_irq_restore(irq);
#else
	spin_unlock_irqrestore(&ring->spinlock, irq);
#endif
	return ret;
}
