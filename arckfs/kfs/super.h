#ifndef SUFS_KFS_SUPER_H_
#define SUFS_KFS_SUPER_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dax.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/memory.h>

#include "../include/kfs_config.h"
#include "../include/common_inode.h"
#include "dev_dax.h"
#include "lease.h"

#define SUFS_KFS_UNOWNED              0
#define SUFS_KFS_WRITE_OWNED          1
#define SUFS_KFS_READ_OWNED           2

struct sufs_pm_node_info {
    /* [start_block, end_block] */
    unsigned long start_block, end_block;
};



struct sufs_super_block
{
    int pm_nodes;
    struct sufs_pm_node_info pm_node_info[SUFS_PM_MAX_INS];
    int head_node;

    unsigned long start_virt_addr;
    unsigned long end_virt_addr;
    unsigned long max_block;

    unsigned long tot_bytes;

    int sockets;

    int cpus_per_socket;

    int dele_ring_per_node;

    struct sufs_free_list *free_lists;

    struct sufs_shadow_inode * sinode_start;

    struct sufs_inode_free_list * inode_free_lists;

    unsigned long head_reserved_blocks;
};

extern struct sufs_super_block sufs_sb;
extern int sufs_kfs_delegation;
extern int sufs_kfs_checker; 

void sufs_init_sb(void);

void sufs_sb_update_one_dev(int node, unsigned long virt_addr,
        unsigned long size_in_bytes);

void sufs_sb_update_devs(struct sufs_dev_arr * sufs_dev_arr);

long sufs_mount(void);

int sufs_umount(unsigned long addr);

long sufs_debug_read(void);

int sufs_fs_init(int soft);

void sufs_fs_fini(void);

int sufs_checker_map(unsigned long arg);

#endif /* KFS_SUPER_H_ */
