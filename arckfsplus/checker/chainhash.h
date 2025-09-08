#ifndef SUFS_CHECKER_CHAINHASH_H_
#define SUFS_CHECKER_CHAINHASH_H_

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#include "../include/checker_config.h"

struct sufs_checker_ch_item
{
    struct sufs_checker_ch_item *next;
    char *key;
    unsigned long key_int;
};

struct sufs_checker_ch_bucket
{
    pthread_spinlock_t lock __attribute__((aligned(SUFS_CACHELINE)));
    struct sufs_checker_ch_item *head;
};

struct sufs_checker_chainhash
{
    unsigned long nbuckets_;
    int is_key_string;
    struct sufs_checker_ch_bucket *buckets_;
};

static inline unsigned long sufs_checker_hash_int(unsigned long v)
{
    unsigned long x = v ^ (v >> 32) ^ (v >> 20) ^ (v >> 12);
    return x ^ (x >> 7) ^ (x >> 4);
}

static inline unsigned long sufs_checker_hash_string(char *string, int max_size)
{
    unsigned long h = 0;
    for (int i = 0; i < max_size && string[i]; i++)
    {
        unsigned long c = string[i];
        /* Lifted from dcache.h in Linux v3.3 */
        h = (h + (c << 4) + (c >> 4)) * 11;
    }
    return h;
}

static inline void sufs_checker_chainhash_fini(struct sufs_checker_chainhash *hash)
{
    if (hash->buckets_)
        free(hash->buckets_);

    hash->buckets_ = NULL;
}

void sufs_checker_chainhash_init(struct sufs_checker_chainhash *hash,
                                 unsigned long size, int is_key_string);

bool sufs_checker_chainhash_lookup(struct sufs_checker_chainhash *hash, char *key,
                                   int max_size, unsigned long key_int);

bool sufs_checker_chainhash_check_and_insert(
        struct sufs_checker_chainhash *hash, char *key, int max_size, 
                                            unsigned long key_int);

void sufs_checker_chainhash_reinit(
    struct sufs_checker_chainhash *hash);

#endif /* CHAINHASH_H_ */
