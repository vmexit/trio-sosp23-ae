#ifndef SUFS_LIBFS_RWLOCK_BRAVO_H_
#define SUFS_LIBFS_RWLOCK_BRAVO_H_

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#include "../../include/libfs_config.h"
#include "trwlock.h"
#include "bravo.h"
#include "util.h"


struct sufs_libfs_bravo_rwlock
{
    bool rbias;
    unsigned long inhibit_until;
    sufs_libfs_rwticket underlying;
};

static inline void
sufs_libfs_bravo_rwlock_init(struct sufs_libfs_bravo_rwlock *l)
{
    l->rbias = true;
    l->inhibit_until = 0;
    l->underlying.u = 0;
}

static void inline
sufs_libfs_bravo_rwlock_destroy(struct sufs_libfs_bravo_rwlock *l)
{

}

static void inline
sufs_libfs_bravo_write_unlock(struct sufs_libfs_bravo_rwlock *l)
{
    sufs_libfs_rwticket_wrunlock(&l->underlying);
}


static inline long  
sufs_libfs_bravo_read_lock(struct sufs_libfs_bravo_rwlock *l)
{
    unsigned long now_time = 0; 
    long slot = 0; 

    if (l->rbias)
    {
        slot = sufs_libfs_bravo_hash((unsigned long) l);

        if (__sync_bool_compare_and_swap(&sufs_libfs_global_vr_table[slot], NULL, l))
        {
            if (l->rbias)
            {
                return slot;
            }

            sufs_libfs_global_vr_table[slot] = NULL;
        }
    }

    /* slow-path */
    sufs_libfs_rwticket_rdlock(&l->underlying);

    now_time = sufs_libfs_rdtsc();

    if (l->rbias == false && now_time >= l->inhibit_until)
    {
        l->rbias = true;
    }
}


static void inline 
sufs_libfs_bravo_read_unlock(struct sufs_libfs_bravo_rwlock *l, 
        long slot)
{
    if (slot == -1)
    {
        slot = sufs_libfs_bravo_hash((unsigned long) l);
    }

    if (sufs_libfs_global_vr_table[slot] != NULL)
    {
        sufs_libfs_global_vr_table[slot] = NULL;
    }
    else
    {
        sufs_libfs_rwticket_rdunlock(&l->underlying);
    }
}

static void inline 
sufs_libfs_bravo_write_lock(struct sufs_libfs_bravo_rwlock *l)
{
    sufs_libfs_rwticket_wrlock(&l->underlying);

    if (l->rbias)
    {
        unsigned long start_time = 0, now_time = 0, i = 0;

        l->rbias = false;

        start_time = sufs_libfs_rdtsc();

        for (i = 0; i < SUFS_LIBFS_RL_NUM_SLOT; i++)
        {
            while (sufs_libfs_global_vr_table[i] == (unsigned long *) l);
        }

        now_time = sufs_libfs_rdtsc();

        l->inhibit_until = now_time + ((now_time - 
            start_time) * SUFS_LIBFS_BRAVO_N);
    }
}

#endif /* BRAVO_H */
