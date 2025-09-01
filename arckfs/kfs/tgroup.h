#ifndef SUFS_KFS_TGROUP_H_
#define SUFS_KFS_TGROUP_H_

#include <linux/kernel.h>

#include "../include/kfs_config.h"
#include "../include/ring_buffer.h"

extern char * sufs_kfs_pid_to_tgroups;

extern struct sufs_tgroup * sufs_tgroup;

struct sufs_tgroup
{
    int used;
    int max_index;
    unsigned int pid[SUFS_MAX_PROCESS_PER_TGROUP];

    /* lease ring */
    unsigned long * lease_ring_kaddr;
    struct sufs_ring_buffer * lease_ring;

    /* wanted ring */
    unsigned long * wanted_ring_kaddr;
    struct page *   wanted_ring_pg;

    /* map ring */
    unsigned long * map_ring_kaddr;
    struct page *   map_ring_pg;

    /* deadline ring */
    unsigned long * ddl_ring_kaddr;

    struct vm_area_struct * mount_vma;
};

int sufs_kfs_init_tgroup(void);

void sufs_kfs_fini_tgroup(void);

int sufs_kfs_alloc_tgroup(void);

int sufs_kfs_free_tgroup(int tgid);

int sufs_kfs_tgroup_add_process(int tgid, int pid);

int sufs_kfs_tgroup_remove_process(int tgid, int pid);

int sufs_kfs_pid_to_tgid_alloc(int pid);


static inline int sufs_kfs_pid_to_tgid(unsigned int pid, int alloc)
{
    char ret = 0;

    ret = sufs_kfs_pid_to_tgroups[pid];

    if (ret == 0 && alloc)
    {
        ret = sufs_kfs_pid_to_tgid_alloc(pid);
    }

    return ret;
}

static inline struct sufs_tgroup *
sufs_kfs_pid_to_tgroup(unsigned int pid, int alloc)
{
    int tgid = sufs_kfs_pid_to_tgid(pid, alloc);

    if (tgid == 0)
        return NULL;
    else
        return &(sufs_tgroup[tgid]);
}

#define sufs_kfs_tgid_to_tgroup(id) (&(sufs_tgroup[(id)]))


#endif
