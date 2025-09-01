#ifndef SUFS_GLOBAL_RING_BUFFER_H_
#define SUFS_GLOBAL_RING_BUFFER_H_

#include "config.h"

#define SUFS_RBUFFER_AGAIN   1

/*
 * TODO: This needs to be further optimized, we want to minimize the
 * size of copying
 */

/* TODO: verify the correctness of cacheline break */
struct sufs_delegation_request {
    /* read, write */
    int type;

    int zero;

    int flush_cache;

    int sfence;

    unsigned long uaddr, offset, bytes;

    int notify_idx;
    int level;

    unsigned long kidx_ptr;

    char pad[8];
};


/* TODO: verify the correctness of cacheline break */
struct sufs_notifyer
{
    volatile int cnt;
    /* cache line break */
    char pad[60];
};



struct sufs_ring_buffer_entry {
    struct sufs_delegation_request request;
    volatile int valid;
    char pad[60];
};

struct sufs_ring_buffer {
    int num_of_entry, entry_size;
    struct sufs_ring_buffer_entry * kfs_requests;
    struct sufs_ring_buffer_entry * libfs_requests;
    char pad1[40];

    int comsumer_idx;
    char pad2[60];

    int producer_idx;
    char pad3[60];

    volatile int spinlock;
    char pad4[60];
};



#endif
