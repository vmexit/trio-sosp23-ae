#include <stdbool.h>
#include <stdio.h>

#include "../include/checker_config.h"
#include "chainhash.h"

static inline void
sufs_checker_ch_item_init(struct sufs_checker_ch_item *item,
                          char *key, unsigned long size,
                          unsigned long key_int, int is_key_string)
{
    if (is_key_string)
    {
        size = strlen(key) + 1;
        item->key = malloc(size);

        strcpy(item->key, key);
    }
    else
    {
        item->key = NULL; 
        item->key_int = key_int; 
    }
}

static inline void sufs_checker_ch_item_free(struct sufs_checker_ch_item *item)
{
    if (item->key)
    {
        free(item->key);
    }

    if (item)
    {
        free(item);
    }
}

static inline void
sufs_checker_chainhash_bucket_init(struct sufs_checker_ch_bucket *bucket)
{
    pthread_spin_init(&(bucket->lock), PTHREAD_PROCESS_SHARED);
    bucket->head = NULL;
}

void sufs_checker_chainhash_init(struct sufs_checker_chainhash *hash, 
    unsigned long size, int is_key_string)
{
    int i = 0;

    hash->nbuckets_ = size;
    hash->buckets_ = malloc(hash->nbuckets_ *
                            sizeof(struct sufs_checker_ch_bucket));

    hash->is_key_string = is_key_string; 

    for (i = 0; i < hash->nbuckets_; i++)
    {
        sufs_checker_chainhash_bucket_init(&(hash->buckets_[i]));
    }
}

static struct sufs_checker_ch_bucket *
sufs_checker_get_buckets(struct sufs_checker_chainhash *hash, char *key, 
                        int max_size, unsigned long key_int)
{
    struct sufs_checker_ch_bucket *buckets = hash->buckets_;
    unsigned long nbuckets = hash->nbuckets_, index = 0;

    if (hash->is_key_string)
    {
        index = sufs_checker_hash_string(key, max_size) % nbuckets;
    }
    else 
    {
        index = sufs_checker_hash_int(key_int) % nbuckets;
    }

    return (&(buckets[index]));
}

bool sufs_checker_chainhash_lookup(struct sufs_checker_chainhash *hash, 
                                   char *key, int max_size, 
                                   unsigned long key_int)
{
    struct sufs_checker_ch_item *i = NULL;
    struct sufs_checker_ch_bucket *b = NULL;
    int is_key_string = hash->is_key_string; 
    bool ret;

    b = sufs_checker_get_buckets(hash, key, max_size, key_int);

    pthread_spin_lock(&b->lock);

    for (i = b->head; i != NULL; i = i->next)
    {
        if ((is_key_string && (strcmp(i->key, key) != 0)) || 
            (!is_key_string && i->key_int != key_int))
        {
            continue;
        }

        ret = true;
        goto out;
    }

    ret = false;

out:
    pthread_spin_unlock(&b->lock);
    return ret;
}

bool sufs_checker_chainhash_check_and_insert(struct 
    sufs_checker_chainhash *hash, char *key, int max_size, 
    unsigned long key_int)
{
    bool ret = false;
    struct sufs_checker_ch_item *i = NULL;
    struct sufs_checker_ch_bucket *b = NULL;
    int is_key_string = hash->is_key_string;

    b = sufs_checker_get_buckets(hash, key, max_size, key_int);

    pthread_spin_lock(&b->lock);

    for (i = b->head; i != NULL; i = i->next)
    {
        if ((is_key_string && (strcmp(i->key, key) == 0)) ||
            (!is_key_string && (i->key_int == key_int)))
        {
            goto out;
        }
    }

    i = malloc(sizeof(struct sufs_checker_ch_item));

    sufs_checker_ch_item_init(i, key, max_size, key_int, hash->is_key_string);

    i->next = b->head;
    b->head = i;

    ret = true;
out:
    pthread_spin_unlock(&b->lock);
    return ret;
}

void sufs_checker_chainhash_reinit(
    struct sufs_checker_chainhash *hash)
{
    int i = 0;

    for (i = 0; i < hash->nbuckets_; i++)
        pthread_spin_lock(&hash->buckets_[i].lock);

    for (i = 0; i < hash->nbuckets_; i++)
    {
        struct sufs_checker_ch_item *iter = hash->buckets_[i].head;
        while (iter)
        {
            struct sufs_checker_ch_item *prev = iter;
            iter = iter->next;
            sufs_checker_ch_item_free(prev);
        }

        hash->buckets_[i].head = NULL;
    }

    for (i = 0; i < hash->nbuckets_; i++)
        pthread_spin_unlock(&hash->buckets_[i].lock);
}

