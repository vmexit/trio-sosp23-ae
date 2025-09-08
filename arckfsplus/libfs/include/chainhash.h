#ifndef SUFS_LIBFS_CHAINHASH_H_
#define SUFS_LIBFS_CHAINHASH_H_

#include <sys/syscall.h>
#include <unistd.h>

#define _LGPL_SOURCE
#define URCU_INLINE_SMALL_FUNCTIONS

#include <urcu/urcu-qsbr.h>
#include <urcu/rculist.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#include "../../include/libfs_config.h"
#include "tls.h"
#include "compiler.h"
#include "types.h"
#include "error_code.h"


// Notify the RCU runtime of a quiescent state every 1000 calls
#define URCU_GP_COUNT_DO_NOTIFY 1000
#define URCU_FREE_BATCH_SIZE 1024

extern const long sufs_libfs_hash_max_size;

struct sufs_libfs_ch_item
{
        struct sufs_libfs_ch_item *next;

        char *key;
        unsigned long val;
        unsigned long val2;
};

struct sufs_libfs_ch_bucket
{
        pthread_spinlock_t lock __mpalign__;
        bool dead_;

        struct sufs_libfs_ch_item *head;
};

struct sufs_libfs_chainhash
{
        u64 nbuckets_;
        struct sufs_libfs_ch_bucket * buckets_;


        u64 nbuckets_resize_;
        struct sufs_libfs_ch_bucket * buckets_resize_;

        atomic_long size;
        atomic_int  seq_lock;
        bool dead_;
};

struct sufs_libfs_chainhash_item_free_batch
{
        struct rcu_head rcu_head;  /* For RCU deferred reclamation */
        struct sufs_libfs_ch_item **items;
        int size;
};

typedef void (*sufs_libfs_rename_complete_t)(
        struct sufs_libfs_mnode *mdnew, 
        char * newname, 
        struct sufs_libfs_mnode *mfold, 
        struct sufs_libfs_mnode *mfroadblock,
        struct sufs_libfs_ch_item * item
);

void sufs_libfs_chainhash_init(struct sufs_libfs_chainhash *hash, int index);

void sufs_libfs_chainhash_resize(struct sufs_libfs_chainhash * hash,
        int enlarge, int max_size);

bool sufs_libfs_chainhash_lookup(struct sufs_libfs_chainhash *hash, char *key,
        int max_size, unsigned long *vptr1, unsigned long *vptr2);

int sufs_libfs_chainhash_insert(struct sufs_libfs_chainhash *hash, char *key,
        int max_size, unsigned long val, unsigned long val2,
        struct sufs_libfs_ch_item ** item, int unlock_on_success, 
        struct sufs_libfs_ch_bucket ** bucket_return, 
        struct sufs_libfs_mnode * mnod);

bool sufs_libfs_chainhash_remove(struct sufs_libfs_chainhash *hash, char *key,
    int max_size, void (*callback_upon_success)(void * arg), 
    struct sufs_libfs_mnode * mnode);

bool sufs_libfs_chainhash_replace_from(
        struct sufs_libfs_chainhash *dst,
        char *kdst,
        unsigned long dst_exist,
        struct sufs_libfs_chainhash *src,
        char *ksrc,
        unsigned long vsrc,
        unsigned long vsrc2,
        int max_size,
        struct sufs_libfs_mnode * mdst, 
        struct sufs_libfs_mnode * msrc,
        struct sufs_libfs_mnode * masrc, 
        struct sufs_libfs_mnode * mfroadblock, 
        sufs_libfs_rename_complete_t cb);

bool sufs_libfs_chainhash_remove_and_kill(struct sufs_libfs_chainhash *hash);

void sufs_libfs_chainhash_forced_remove_and_kill(
        struct sufs_libfs_chainhash *hash);

bool sufs_libfs_chainhash_enumerate(struct sufs_libfs_chainhash *hash,
        int max_size, char *prev, char *out);

__ssize_t sufs_libfs_chainhash_getdents(struct sufs_libfs_chainhash *hash,
        int max_size, unsigned long * offset_ptr, void * buffer, size_t length);


static inline struct sufs_libfs_chainhash_item_free_batch *
alloc_free_batch(void)
{
    struct sufs_libfs_chainhash_item_free_batch *batch;

    batch = malloc(sizeof(*batch));
    if (!batch) {
        fprintf(stderr, "Failed to allocate memory for RCU free batch\n");
        abort();
    }

    batch->items = malloc(URCU_FREE_BATCH_SIZE * 
            sizeof(struct sufs_libfs_ch_item *));
    if (!batch->items) {
        fprintf(stderr, "Failed to allocate memory for RCU free batch items\n");
        free(batch);
        abort();
    }

    batch->size = 0;

    return batch;
}

static inline struct sufs_libfs_tls *
urcu_init_via_tls_if_not_done(void) {
    struct sufs_libfs_tls *this_tls = &sufs_libfs_tls_data[sufs_libfs_tls_my_index()];
    int *urcu_init_done = &this_tls->urcu_init_done;

    if (!*urcu_init_done) {

        urcu_qsbr_register_thread();

        this_tls->urcu_gp_batch = alloc_free_batch();

        *urcu_init_done = 1;
    }

    return this_tls;
}

static inline void rcu_read_lock(void) {
    urcu_init_via_tls_if_not_done();

    urcu_qsbr_read_lock();
}

static inline void rcu_read_unlock(void) {
    urcu_qsbr_read_unlock();

    struct sufs_libfs_tls *this_tls;
    this_tls = urcu_init_via_tls_if_not_done();

    this_tls->urcu_gp_count++;

    if (this_tls->urcu_gp_count >= URCU_GP_COUNT_DO_NOTIFY) {
        urcu_qsbr_quiescent_state();
        this_tls->urcu_gp_count = 0;
    }
}

static inline bool sufs_libfs_chainhash_killed(
        struct sufs_libfs_chainhash *hash)
{
    return hash->dead_;
}

static inline void sufs_libfs_chainhash_fini(struct sufs_libfs_chainhash *hash)
{
    if (hash->buckets_)
        free(hash->buckets_);

    hash->dead_ = true;
    hash->buckets_ = NULL;
}


static inline void
sufs_libfs_chainhash_obtain_one_lock(struct sufs_libfs_chainhash *hash) 
{
    pthread_spin_lock(&(hash->buckets_[0].lock));
}

static inline void
sufs_libfs_chainhash_release_one_lock(struct sufs_libfs_chainhash *hash) 
{
    pthread_spin_unlock(&(hash->buckets_[0].lock));
}


static inline void 
sufs_libfs_chainhash_obtain_all_locks(struct sufs_libfs_chainhash *hash)
{
    int i = 0;
    for (i = 0; i < hash->nbuckets_; i++)
    {
        pthread_spin_lock(&(hash->buckets_[i].lock));
    }
}

static inline void 
sufs_libfs_chainhash_release_all_locks(struct sufs_libfs_chainhash *hash)
{
    int i = 0;
    for (i = 0; i < hash->nbuckets_; i++)
    {
        pthread_spin_unlock(&(hash->buckets_[i].lock));
    }
}


static inline void 
sufs_libfs_chainhash_unlock_write_ops(struct sufs_libfs_chainhash *hash, 
        struct sufs_libfs_ch_bucket *b, int max_size)
{
        pthread_spin_unlock(&(b->lock));

        /* TODO: Make the test a function.. */
        if (hash->nbuckets_ != sufs_libfs_hash_max_size &&
        hash->size > hash->nbuckets_ * SUFS_LIBFS_DIR_REHASH_FACTOR)
        {
                sufs_libfs_chainhash_resize(hash, 1, max_size);
        }

        rcu_read_unlock();
}

static inline void
sufs_libfs_chainhash_bucket_init(struct sufs_libfs_ch_bucket * bucket)
{
    pthread_spin_init(&(bucket->lock), PTHREAD_PROCESS_SHARED);

    bucket->head = NULL;
    bucket->dead_ = false;
}

static inline void
sufs_libfs_ch_item_init(struct sufs_libfs_ch_item *item, char *key,
        unsigned long size, unsigned value, unsigned long value2)
{
    size = strlen(key) + 1;
    item->key = malloc(size);
    strcpy(item->key, key);

    item->val = value;
    item->val2 = value2;
}

static inline 
void rcu_free_batch(struct rcu_head *head)
{
    struct sufs_libfs_chainhash_item_free_batch *batch = 
        container_of(head, struct sufs_libfs_chainhash_item_free_batch, 
                     rcu_head);

    int size = batch->size;
    struct sufs_libfs_ch_item **iter = batch->items;

    if (size > URCU_FREE_BATCH_SIZE) {
        fprintf(stderr, "Batch size is too large: %d, at %p\n", size, batch);
        abort();
    }

    for (int i = 0; i < size; i++, iter++) {
        struct sufs_libfs_ch_item *item = *iter;
        if (!item) {
            fprintf(stderr, "NULL item in RCU free batch\n");
            continue;
        }

        if (item->key)
            free(item->key);
        free(item);
    }

    free(batch);
}

static inline void 
rcu_free(struct sufs_libfs_ch_item *item) 
{
    struct sufs_libfs_tls *this_tls;
    this_tls = urcu_init_via_tls_if_not_done();
    struct sufs_libfs_chainhash_item_free_batch *
        batch = this_tls->urcu_gp_batch;

    u64 size = batch->size;

    unsigned long rand_threshold = ((unsigned long) this_tls >> 2) * 997 % 256;

    if (size + rand_threshold < URCU_FREE_BATCH_SIZE) {
        batch->items[size++] = item;
        batch->size = size;
        return;
    } else {
        this_tls->urcu_gp_batch = alloc_free_batch();
        this_tls->urcu_gp_batch->items[0] = item; 
        this_tls->urcu_gp_batch->size = 1; 
        urcu_qsbr_call_rcu(&batch->rcu_head, rcu_free_batch);
    }
}



#endif /* CHAINHASH_H_ */
