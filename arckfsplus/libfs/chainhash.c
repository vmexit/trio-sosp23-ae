#include <stdbool.h>
#include <stdio.h>

#include <sys/syscall.h>
#include <unistd.h>

#define _LGPL_SOURCE
#define URCU_INLINE_SMALL_FUNCTIONS

#include <urcu/urcu-qsbr.h>
#include <urcu/rculist.h>

#include "../include/libfs_config.h"
#include "chainhash.h"
#include "hash.h"
#include "ialloc.h"
#include "syscall.h"
#include "seqlock.h"
#include "mfs.h"

#define SUFS_LIBFS_HASH_SIZES_MAX  15

static const long sufs_libfs_hash_sizes[SUFS_LIBFS_HASH_SIZES_MAX] =
{
    1063,
    2153,
    4363,
    8219,
    16763,
    32957,
    64601,
    128983,
    256541,
    512959,
    1024921,
    2048933,
    4096399,
    8192003,
    16384001
};

const long sufs_libfs_hash_min_size =
        sufs_libfs_hash_sizes[0];

const long sufs_libfs_hash_max_size =
        sufs_libfs_hash_sizes[SUFS_LIBFS_HASH_SIZES_MAX - 1];








/*
 * value : ino
 * value2: ptr to struct sufs_dir_entry
 */




void sufs_libfs_chainhash_init(struct sufs_libfs_chainhash *hash, int index)
{
    urcu_init_via_tls_if_not_done();

    int i = 0;

    if (index < 0 || index >= SUFS_LIBFS_HASH_SIZES_MAX)
    {
        fprintf(stderr, "index :%d for hash table is too large!\n", index);
        abort();
    }

    u64 nbuckets = sufs_libfs_hash_sizes[index];

    hash->nbuckets_ = nbuckets;
    hash->buckets_ = malloc(hash->nbuckets_ *
            sizeof(struct sufs_libfs_ch_bucket));

    hash->nbuckets_resize_ = 0;
    hash->buckets_resize_ = NULL;

    hash->dead_ = false;
    hash->size = 0;

    sufs_libfs_seq_lock_init(&(hash->seq_lock));

    for (i = 0; i < hash->nbuckets_; i++)
    {
        sufs_libfs_chainhash_bucket_init(&(hash->buckets_[i]));
    }
}

struct sufs_libfs_ch_bucket *
sufs_libfs_chainhash_find_resize_buckets(struct sufs_libfs_chainhash *hash,
        char * key, int max_size)
{
    struct sufs_libfs_ch_bucket *buckets = NULL;
    u64 nbuckets = 0;
    struct sufs_libfs_ch_bucket *b = NULL;

    buckets = hash->buckets_resize_;
    nbuckets = hash->nbuckets_resize_;

    /* This can happen due to the completion of resize */
    while (buckets == NULL || nbuckets == 0)
    {
        buckets = hash->buckets_;
        nbuckets = hash->nbuckets_;
    }

    b = &(buckets[sufs_libfs_hash_string(key, max_size) % nbuckets]);

    return b;
}

static struct sufs_libfs_ch_bucket *
sufs_libfs_get_buckets(struct sufs_libfs_chainhash * hash, char * key, int max_size)
{
    int bseq = 0, eseq = 0;
    struct sufs_libfs_ch_bucket * buckets = NULL;
    u64 nbuckets = 0;

    do
    {
        bseq = sufs_libfs_seq_lock_read(&(hash->seq_lock));

        buckets = hash->buckets_;
        nbuckets = hash->nbuckets_;

        eseq = sufs_libfs_seq_lock_read(&(hash->seq_lock));

    } while (sufs_libfs_seq_lock_retry(bseq, eseq));

    return (&(buckets[sufs_libfs_hash_string(key, max_size) % nbuckets]));
}

bool sufs_libfs_chainhash_lookup(struct sufs_libfs_chainhash *hash, char *key,
        int max_size, unsigned long *vptr, unsigned long *vptr2)
{
    struct sufs_libfs_ch_item *i = NULL;
    struct sufs_libfs_ch_bucket *b = NULL;
    bool ret = false;

    rcu_read_lock();

    b = sufs_libfs_get_buckets(hash, key, max_size);

    if (b->dead_)
    {
        b = sufs_libfs_chainhash_find_resize_buckets(hash, key, max_size);
    }

    /* RCU-protected traversal of the chain */
    for (i = rcu_dereference(b->head); i != NULL; i = rcu_dereference(i->next))
    {
        if (strcmp(i->key, key) != 0)
            continue;

        if (vptr)
            (*vptr) = i->val;

        if (vptr2)
            (*vptr2) = i->val2;

        ret = true;
        break;
    }

    rcu_read_unlock();
    return ret;
}

static unsigned long
sufs_libfs_chainhash_new_size(struct sufs_libfs_chainhash * hash, int enlarge)
{
    int i = 0;

    for (i = 0; i < SUFS_LIBFS_HASH_SIZES_MAX; i++)
    {
        if (sufs_libfs_hash_sizes[i] == hash->nbuckets_)
            break;
    }

    if (enlarge)
    {
        if (i == SUFS_LIBFS_HASH_SIZES_MAX)
        {
            fprintf(stderr, "Hash reaches maximum size!\n");
            return 0;
        }

        i++;
    }
    else
    {
        if (i == 0)
        {
            fprintf(stderr, "Bug: reducing the size of a minimum hash!\n");
            return 0;
        }

        i--;
    }

    return sufs_libfs_hash_sizes[i];
}

void sufs_libfs_chainhash_resize(struct sufs_libfs_chainhash * hash,
        int enlarge, int max_size)
{
    int i = 0;

#if 0
    return;
#endif

    /* The resize is already in progress, return */
    if (!__sync_bool_compare_and_swap(&hash->nbuckets_resize_, 0, 1))
        return;

    hash->nbuckets_resize_= sufs_libfs_chainhash_new_size(hash, enlarge);

    if (!hash->nbuckets_resize_)
    {
        return;
    }

    /* init the resize hash */
    hash->buckets_resize_ = malloc(hash->nbuckets_resize_ *
            sizeof(struct sufs_libfs_ch_bucket));

    if (!hash->buckets_resize_)
    {
        fprintf(stderr, "Cannot malloc the new hash table");
        abort();
    }

    for (i = 0; i < hash->nbuckets_resize_; i++)
    {
        sufs_libfs_chainhash_bucket_init(&(hash->buckets_resize_[i]));
    }


    /* move the entry in the old hash to the new one */
    for (i = 0; i < hash->nbuckets_; i++)
    {
        struct sufs_libfs_ch_item *iter = NULL;
        struct sufs_libfs_ch_bucket *b = NULL;

        b = &(hash->buckets_[i]);

        pthread_spin_lock(&(b->lock));
        b->dead_ = true;

        iter = b->head;
        while (iter)
        {
            struct sufs_libfs_ch_item *prev = NULL;
            struct sufs_libfs_ch_bucket *nb = NULL;

            prev = iter;
            iter = iter->next;

            nb =  &hash->buckets_resize_[sufs_libfs_hash_string(prev->key, 
                    max_size) % hash->nbuckets_resize_];

            pthread_spin_lock(&(nb->lock));

            /* RCU-safe insertion into new bucket */
            prev->next = nb->head;
            rcu_assign_pointer(nb->head, prev);

            pthread_spin_unlock(&(nb->lock));
        }

        rcu_assign_pointer(b->head, NULL);

        pthread_spin_unlock(&(b->lock));
    }

    /* swap back */
    sufs_libfs_seq_lock_write_begin(&hash->seq_lock);

    hash->buckets_ = hash->buckets_resize_;
    hash->nbuckets_ = hash->nbuckets_resize_;

    sufs_libfs_seq_lock_write_end(&hash->seq_lock);

    hash->buckets_resize_ = NULL;
    hash->nbuckets_resize_ = 0;

    // Free some RCU batch here to avoid resonance.
    struct sufs_libfs_tls *this_tls;
    this_tls = urcu_init_via_tls_if_not_done();
    struct sufs_libfs_chainhash_item_free_batch *batch = this_tls->urcu_gp_batch;
    if (batch->size > URCU_FREE_BATCH_SIZE / 2) {
        urcu_qsbr_call_rcu(&batch->rcu_head, rcu_free_batch);
        this_tls->urcu_gp_batch = alloc_free_batch();
    }

    return;
}

int sufs_libfs_chainhash_insert(struct sufs_libfs_chainhash *hash, char *key,
        int max_size, unsigned long val, unsigned long val2,
        struct sufs_libfs_ch_item ** item, int unlock_on_success, 
        struct sufs_libfs_ch_bucket ** bucket_return, 
        struct sufs_libfs_mnode * mnode)
{
    int ret = SUFS_LIBFS_ERR_FAIL;
    struct sufs_libfs_ch_item *i = NULL;
    struct sufs_libfs_ch_bucket *b = NULL;

    if (hash->dead_)
    {
        return false;
    }

    rcu_read_lock();

    b = sufs_libfs_get_buckets(hash, key, max_size);
    pthread_spin_lock(&b->lock);

    if (hash->dead_)
    {
        ret = false;
        goto out;
    }

    if (b->dead_)
    {
        pthread_spin_unlock(&b->lock);
        b = sufs_libfs_chainhash_find_resize_buckets(hash, key, max_size);
        pthread_spin_lock(&b->lock);
    }

    if (!sufs_libfs_file_is_mapped(mnode))
    {
        ret = false; 
        goto out; 
    }

    /* Check if key already exists */
    for (i = b->head; i != NULL; i = i->next)
    {
        if (strcmp(i->key, key) == 0)
        {
            ret = SUFS_LIBFS_ERR_ALREADY_EXIST;
            if (item)
            {
                *item = i;
            }
            goto out;
        }
    }

    /* Allocate new item */
    i = malloc(sizeof(struct sufs_libfs_ch_item));
    if (!i) {
        ret = false;
        goto out;
    }

    sufs_libfs_ch_item_init(i, key, max_size, val, val2);

    /* RCU publish: update next pointer first, then head pointer */
    i->next = b->head;
    rcu_assign_pointer(b->head, i);

    ret = SUFS_LIBFS_ERR_SUCCESS;

    if (item)
        *item = i;

    hash->size++;


out:
    if (ret == SUFS_LIBFS_ERR_SUCCESS)
    {
        if (unlock_on_success)
        {
            sufs_libfs_chainhash_unlock_write_ops(hash, b, max_size);
        }
        else if (bucket_return)
        {
            (*bucket_return) = b;
        }
    }
    else
    {
        sufs_libfs_chainhash_unlock_write_ops(hash, b, max_size);
    }

    return ret;
}



bool sufs_libfs_chainhash_remove(struct sufs_libfs_chainhash *hash, char *key,
    int max_size, void (*callback_upon_success)(void * arg), 
    struct sufs_libfs_mnode * mnode)
{
    bool ret = false;
    struct sufs_libfs_ch_bucket *b = NULL;
    struct sufs_libfs_ch_item *i = NULL, *prev = NULL;

    rcu_read_lock();

    b = sufs_libfs_get_buckets(hash, key, max_size);
    pthread_spin_lock(&b->lock);

    if (b->dead_)
    {
        pthread_spin_unlock(&b->lock);
        b = sufs_libfs_chainhash_find_resize_buckets(hash, key, max_size);
        pthread_spin_lock(&b->lock);
    }

    if (!sufs_libfs_file_is_mapped(mnode))
        goto out; 

    /* Find and unlink the item */
    for (i = b->head; i != NULL; prev = i, i = i->next)
    {
        if (strcmp(i->key, key) == 0)
        {
            /* Unlink from chain using RCU-safe assignment */
            if (prev == NULL)
            {
                rcu_assign_pointer(b->head, i->next);
            }
            else
            {
                rcu_assign_pointer(prev->next, i->next);
            }

            ret = true;
            hash->size--;
            break;
        }
    }

    if (ret && callback_upon_success)
    {
        callback_upon_success((void *) i->val2);
    }

out: 
    pthread_spin_unlock(&b->lock);

    if (ret)
    {
        rcu_free(i);
    }

    if (ret && hash->nbuckets_ != sufs_libfs_hash_min_size &&
            hash->size * SUFS_LIBFS_DIR_REHASH_FACTOR < hash->nbuckets_)
    {
        sufs_libfs_chainhash_resize(hash, 0, max_size);
    }

    rcu_read_unlock();

    return ret;
}

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
        sufs_libfs_rename_complete_t cb)
{
    int index = 0, i = 0, j = 0;
    struct sufs_libfs_ch_bucket *bdst = NULL;

    struct sufs_libfs_ch_bucket *bsrc = NULL;

    struct sufs_libfs_ch_item *srci = NULL, *srcprev = NULL, *dsti = NULL,
            *item_tmp = NULL, * item = NULL;

    bool ret = 0, free_src = 0;

    struct sufs_libfs_ch_bucket *buckets[2];

    rcu_read_lock();

    bdst = sufs_libfs_get_buckets(dst, kdst, max_size);
    bsrc = sufs_libfs_get_buckets(src, ksrc, max_size);

lock:
    index = 0;

    if (bsrc != bdst)
    {
        buckets[index] = bsrc;
        index++;
    }

    buckets[index] = bdst;
    index++;

    for (i = 0; i < index - 1; i++)
    {
        for (j = i + 1; j < index; j++)
        {
            if ((unsigned long) buckets[j] < (unsigned long) buckets[i])
            {
                struct sufs_libfs_ch_bucket *tmp;
                tmp = buckets[j], buckets[j] = buckets[i], buckets[i] = tmp;
            }
        }
    }

    for (i = 0; i < index; i++)
    {
        pthread_spin_lock(&buckets[i]->lock);
    }

    if (bsrc->dead_)
    {
        for (i = 0; i < index; i++)
            pthread_spin_unlock(&(buckets[i]->lock));
        bsrc = sufs_libfs_chainhash_find_resize_buckets(src, ksrc, max_size);
        goto lock;
    }

    if (bdst->dead_)
    {
        for (i = 0; i < index; i++)
            pthread_spin_unlock(&(buckets[i]->lock));
        bdst = sufs_libfs_chainhash_find_resize_buckets(dst, kdst, max_size);
        goto lock;
    }

    if (dst->dead_)
    {
        ret = false;
        goto out;
    }


    /* Find the source */
    srci = bsrc->head;
    srcprev = NULL;
    while (1)
    {
        if (srci == NULL)
        {
            ret = false;
            goto out;
        }

        if (strcmp(srci->key, ksrc) == 0)
        {
            break;
        }

        srcprev = srci;
        srci = srci->next;
    }

    if (!sufs_libfs_file_is_mapped(msrc) 
        || !sufs_libfs_file_is_mapped(mdst))
    {
        ret = false;
        goto out;
    }

    /* Find the destination */
    dsti = bdst->head;

    while (dsti != NULL)
    {
        if (strcmp(dsti->key, kdst) == 0)
        {
            if (!dst_exist)
            {
                ret = false;
                goto out;
            }

            dsti->val = vsrc;
            dsti->val2 = vsrc2;

            item = dsti;

            if (srcprev == NULL)
            {
                rcu_assign_pointer(bsrc->head, srci->next);
            }
            else
            {
                rcu_assign_pointer(srcprev->next, srci->next);
            }

            free_src = 1;
            ret = true;
            goto out;
        }

        dsti = dsti->next;
    }

    if (dst_exist)
    {
        ret = false;
        goto out;
    }

    if (srcprev == NULL)
    {
        rcu_assign_pointer(bsrc->head, srci->next);
    }
    else
    {
        rcu_assign_pointer(srcprev->next, srci->next);
    }

    free_src = 1;

    item_tmp = malloc(sizeof(struct sufs_libfs_ch_item));

    sufs_libfs_ch_item_init(item_tmp, kdst, max_size, vsrc, vsrc2);

    item_tmp->next = bdst->head;
    rcu_assign_pointer(bdst->head, item_tmp);

    ret = true;

    item = item_tmp;

out:
    if (ret)
    {
        cb(mdst, kdst, masrc, mfroadblock, item);
    }

    for (i = 0; i < index; i++)
        pthread_spin_unlock(&(buckets[i]->lock));

    if (free_src)
        rcu_free(srci);

    src->size--;
    dst->size++;

    if (ret && src->nbuckets_!= sufs_libfs_hash_min_size &&
            src->size * SUFS_LIBFS_DIR_REHASH_FACTOR < src->nbuckets_)
    {
        sufs_libfs_chainhash_resize(src, 0, max_size);
    }

    if (ret && dst->nbuckets_ != sufs_libfs_hash_max_size &&
            dst->size > dst->nbuckets_ * SUFS_LIBFS_DIR_REHASH_FACTOR)
    {
        sufs_libfs_chainhash_resize(dst, 1, max_size);
    }

    rcu_read_unlock();

    return ret;
}

bool sufs_libfs_chainhash_remove_and_kill(struct sufs_libfs_chainhash *hash)
{
    if (hash->dead_ || hash->size != 0)
        return false;

    hash->dead_ = true;

    return true;
}

/* This will not be executed concurrently with resize so we are good */
void sufs_libfs_chainhash_forced_remove_and_kill(
        struct sufs_libfs_chainhash *hash)
{
    int i = 0;

    for (i = 0; i < hash->nbuckets_; i++)
        pthread_spin_lock(&hash->buckets_[i].lock);

    for (i = 0; i < hash->nbuckets_; i++)
    {
        struct sufs_libfs_ch_item *iter = hash->buckets_[i].head;
        while (iter)
        {
            struct sufs_libfs_ch_item *prev = iter;

            sufs_libfs_inode_clear_allocated(iter->val);

            iter = iter->next;

            /* Direct free since we're already synchronized */
            if (prev->key)
                free(prev->key);
            free(prev);
        }
    }

    hash->dead_ = true;

    for (i = 0; i < hash->nbuckets_; i++)
        pthread_spin_unlock(&hash->buckets_[i].lock);
}


bool sufs_libfs_chainhash_enumerate(struct sufs_libfs_chainhash *hash,
        int max_size, char *prev, char *out)
{
    u64 i = 0;
    bool prevbucket = (prev != NULL);

    i = prev ? sufs_libfs_hash_string(prev, max_size) % hash->nbuckets_ :
             0;

    rcu_read_lock();

    for (; i < hash->nbuckets_; i++)
    {
        struct sufs_libfs_ch_bucket *b = &hash->buckets_[i];
        struct sufs_libfs_ch_item *item;
        bool found = false;

        for (item = rcu_dereference(b->head); item != NULL; 
             item = rcu_dereference(item->next))
        {
            if (prevbucket)
            {
                if (strcmp(item->key, prev) == 0)
                    prevbucket = false;
            }
            else
            {
                strcpy(out, item->key);
                found = true;
                break;
            }
        }

        if (found)
            return true;

        prevbucket = false;
    }

    rcu_read_unlock();

    return false;
}


__ssize_t sufs_libfs_chainhash_getdents(struct sufs_libfs_chainhash *hash,
        int max_size, unsigned long * offset_ptr, void * buffer, size_t length)
{
    unsigned long offset = (*offset_ptr);

    struct sufs_dir_entry * dir = NULL;
    unsigned long count = 0;

    /*
     * MS 32 bits: which bucket,
     * LS 32 bits: offset with in the bucket
     */

    u64 i = offset >> 32;
    u64 j = offset & 0xffffffff;

    struct linux_dirent64 * iter_ptr = buffer;
    size_t iter_offset = 0;

    rcu_read_lock();

    for (; i < hash->nbuckets_; i++)
    {
        count = 0;
        struct sufs_libfs_ch_bucket *b = &hash->buckets_[i];
        struct sufs_libfs_ch_item *item = NULL;

        for (item = rcu_dereference(b->head); item != NULL; item = rcu_dereference(item->next))
        {
            if (j > 0)
            {
                j--;
            }
            else
            {
                long dent_size = 0;
                dir = (struct sufs_dir_entry *) item->val2;

                dent_size = sizeof(struct linux_dirent64) + dir->name_len - 1;

                if (iter_offset + dent_size > length)
                {
                    goto out;
                }
                iter_ptr->d_ino = item->val;
                iter_ptr->d_off = iter_ptr->d_reclen = dent_size;

                if (dir->inode.file_type == SUFS_FILE_TYPE_REG)
                    iter_ptr->d_type = DT_REG;
                else
                    iter_ptr->d_type = DT_DIR;

                strcpy(iter_ptr->d_name, dir->name);

                iter_offset += dent_size;
                iter_ptr = (struct linux_dirent64 *)
                        ((unsigned long) buffer + iter_offset);
            }

            count++;
        }

        j = 0;
    }

out:
    (*offset_ptr) = ((i << 32) | count);

    rcu_read_unlock();

    return iter_offset;
}
