#ifndef __SIMPLE_RING_H_
#define __SIMPLE_RING_H_

#include <linux/spinlock.h>

#include "delegation.h"
#include "pmfs_config.h"

struct spmfs_ring_buffer_entry {
	/* This is not perfect, but keep it for the sake of simplicity */
	struct pmfs_delegation_request request;
	volatile int valid;
};

struct spmfs_ring_buffer {
	struct spmfs_ring_buffer_entry *requests;
#if PMFS_RING_SIMPLE_LOCK
	volatile int spinlock;
#else
	spinlock_t spinlock;
#endif
	int producer_idx, comsumer_idx, num_of_entry, entry_size;
};

struct spmfs_ring_buffer *pmfs_sr_create(int socket, int entry_size, int size);

void pmfs_sr_destroy(struct spmfs_ring_buffer *ring);

int pmfs_sr_send_request(struct spmfs_ring_buffer *ring, void *from);

int pmfs_sr_receive_request(struct spmfs_ring_buffer *ring, void *to);

#endif
