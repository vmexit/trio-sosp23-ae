#ifndef SUFS_GLOBAL_PRIV_INODE_H_
#define SUFS_GLOBAL_PRIV_INODE_H_

#include "config.h"

struct sufs_kfs_lease
{
    volatile int lock;

    int state, owner_cnt;
    int owner[SUFS_KFS_LEASE_MAX_OWNER];
    unsigned long lease_tsc[SUFS_KFS_LEASE_MAX_OWNER];
};

/* TODO: add pad space to make it cache block aligned */
struct sufs_shadow_inode
{
    char file_type;
    unsigned int mode;
    unsigned int uid;
    unsigned int gid;

    unsigned long index_offset;

    unsigned long shadow_index_offset;

    struct sufs_kfs_lease lease;
    
    int parent; 
};

#endif 