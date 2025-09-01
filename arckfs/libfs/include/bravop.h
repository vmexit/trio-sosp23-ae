
#ifndef SUFS_LIBFS_BRAVOP_H_
#define SUFS_LIBFS_BRAVOP_H_

#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include "../../include/libfs_config.h"
#include "../../include/common_inode.h"
#include "util.h" 
#include "tls.h"

extern int sufs_libfs_alloc_cpu;

// extern long sufs_libfs_ncores;

struct sufs_libfs_per_core_lock
{
    pthread_spinlock_t lock;
    char pad[SUFS_CACHELINE - sizeof(pthread_rwlock_t)];
} __attribute__((aligned(SUFS_CACHELINE)));

struct sufs_libfs_bravop_rwlock
{
    unsigned long rbias;
    unsigned long inhibit_until;
    pthread_spinlock_t underlying;
    char pad1[SUFS_CACHELINE - (sizeof(unsigned long) * 2 + sizeof(pthread_spinlock_t))];

    struct sufs_libfs_per_core_lock *locks;
    char pad2[SUFS_CACHELINE - sizeof(struct sufs_libfs_per_core_lock *)];
} __attribute__((aligned(SUFS_CACHELINE)));

static inline int
sufs_libfs_bravop_init(struct sufs_libfs_bravop_rwlock *l)
{
    long i = 0, sz = 0;

    l->rbias = 1; 
    l->inhibit_until = 0;

    pthread_spin_init(&(l->underlying), PTHREAD_PROCESS_SHARED);

    sz = sizeof(struct sufs_libfs_per_core_lock) * sufs_libfs_alloc_cpu;
    if (posix_memalign((void **)&l->locks, SUFS_CACHELINE, sz) != 0)
        return -1;

    for (i = 0; i < sufs_libfs_alloc_cpu; i++) 
    {
        pthread_spin_init(&(l->locks[i].lock), PTHREAD_PROCESS_SHARED);
    }
    return 0;
}

static inline void
sufs_libfs_bravop_fini(struct sufs_libfs_bravop_rwlock *l)
{
    long i = 0;
    for (i = 0; i < sufs_libfs_alloc_cpu; i++) 
    {
        pthread_spin_destroy(&(l->locks[i].lock));
    }
    free(l->locks);
}

static inline int
sufs_libfs_bravop_read_lock(struct sufs_libfs_bravop_rwlock *l)
{
    unsigned long now_time = 0;
    int cpu = 0; 

    if (l->rbias)
    {
        cpu = sufs_libfs_tls_my_index();
        
        pthread_spin_lock(&(l->locks[cpu].lock));

            if (l->rbias)
                return 1;
        
        pthread_spin_unlock(&(l->locks[cpu].lock));
    }

    pthread_spin_lock(&(l->underlying));

    now_time = sufs_libfs_rdtsc();

    if (l->rbias == 0 && now_time >= l->inhibit_until)
    {
        l->rbias = 1;
    }
    
    return 0;
}

static inline void
sufs_libfs_bravop_read_unlock(struct sufs_libfs_bravop_rwlock *l, int fast)
{
    int cpu = 0; 

    if (fast)
    {
        cpu = sufs_libfs_tls_my_index();

        pthread_spin_unlock(&(l->locks[cpu].lock));
    }
    else
    {
        pthread_spin_unlock(&(l->underlying));    
    }
}

static inline void
sufs_libfs_bravop_write_lock(struct sufs_libfs_bravop_rwlock *l)
{
    long i = 0;
    unsigned long start_time = 0, now_time = 0;

    pthread_spin_lock(&(l->underlying));

    if (l->rbias)
    {
        l->rbias = 0;

        start_time = sufs_libfs_rdtsc();

        for (i = 0; i < sufs_libfs_alloc_cpu; i++) 
        {
            pthread_spin_lock(&(l->locks[i].lock));
            pthread_spin_unlock(&(l->locks[i].lock));
        }

        now_time = sufs_libfs_rdtsc();

        l->inhibit_until = now_time + ((now_time - 
            start_time) * SUFS_LIBFS_BRAVO_N);
    }
}

static inline void
sufs_libfs_bravop_write_unlock(struct sufs_libfs_bravop_rwlock *l)
{
    pthread_spin_unlock(&(l->underlying));
}

void sufs_libfs_init_num_cores(void);

#endif // SUFS_LIBFS_BRAVOP_H_
