#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

#include "../include/kfs_config.h"
#include "util.h"
#include "tgroup.h"
#include "lease.h"
#include "super.h"
#include "inode.h"
#include "mmap.h"

struct sufs_kfs_lease sufs_kfs_rename_lease; 

static struct sufs_kfs_lease* sufs_kfs_get_lease(int ino)
{
    struct sufs_shadow_inode *sinode = NULL;

    sinode = sufs_find_sinode(ino);

    if (sinode == NULL)
    {
        printk("Cannot find sinode with ino %d\n", ino);
        return NULL;
    }

    return &(sinode->lease);
}

static inline int sufs_kfs_lease_need_check_expire(struct sufs_kfs_lease *l,
        int new_state)
{
    return (new_state == SUFS_KFS_WRITE_OWNED
            || l->state == SUFS_KFS_WRITE_OWNED);
}

static void sufs_kfs_gc_lease(struct sufs_kfs_lease *l)
{
    int i = 0;


    pid_t sowner[SUFS_KFS_LEASE_MAX_OWNER];
    unsigned long slease_tsc[SUFS_KFS_LEASE_MAX_OWNER];
    int sowner_cnt = 0;

    for (i = 0; i < l->owner_cnt; i++)
    {
        if (l->owner[i] != 0)
        {
            sowner[sowner_cnt] = l->owner[i];
            slease_tsc[sowner_cnt] = l->lease_tsc[i];
            sowner_cnt++;
        }
    }

    for (i = 0; i < sowner_cnt; i++)
    {
        l->owner[i] = sowner[i];
        l->lease_tsc[i] = slease_tsc[i];
    }

    l->owner_cnt = sowner_cnt;
}

static inline int sufs_kfs_need_mcheck(struct sufs_kfs_lease *l)
{
    if (l->state == SUFS_KFS_UNOWNED || l->state == SUFS_KFS_READ_OWNED)
    {
        return 0;
    }

    return 1;
}

static void 
sufs_kfs_clean_up_force_info(int ino, struct sufs_kfs_lease * l)
{
    int i = 0;

    for (i = 0; i < l->owner_cnt; i++)
    {
        int owner = l->owner[i];
        struct sufs_tgroup * tgroup = sufs_kfs_tgid_to_tgroup(owner);

        l->lease_tsc[i] = 0;

        if (tgroup)
        {
            if (tgroup->ddl_ring_kaddr)
            {
                tgroup->ddl_ring_kaddr[ino] = 0;
            }
        }
    }
}



static int 
sufs_kfs_try_acquire (int ino, struct sufs_kfs_lease *l, 
    struct sufs_shadow_inode * sinode, int tgid, int new_state)
{
    int i = 0, ret = 0;
    struct sufs_tgroup *tgroup = NULL;
    struct vm_area_struct *vma = NULL;

    if (l->state == SUFS_KFS_UNOWNED)
        return ret;

    for (i = 0; i < l->owner_cnt; i++)
    {
        struct sufs_ring_buffer * lease_ring = NULL;
        unsigned long *ddl_ring_addr = NULL;

        if (l->lease_tsc[i] == 0)
        {
            unsigned long * wanted_ring_addr = 
                    sufs_tgroup[l->owner[i]].wanted_ring_kaddr;

            if (wanted_ring_addr)
            {
                set_bit(ino, wanted_ring_addr);
            }
            else
            {
                return 0;
            }
            
            lease_ring = sufs_tgroup[l->owner[i]].lease_ring;

            if (lease_ring)
            {
                sufs_lease_queue_send(lease_ring, ino);
            }

            ddl_ring_addr = sufs_tgroup[l->owner[i]].ddl_ring_kaddr;

            l->lease_tsc[i] = 
                sufs_kfs_rdtsc() + SUFS_LEASE_CYCLES + SUFS_LEASE_GRACE_CYCLES;
            
            if (ddl_ring_addr)
            {
                ddl_ring_addr[ino] = sufs_kfs_rdtsc() + SUFS_LEASE_CYCLES;
            }

            return -EAGAIN;
        }
        else if (sufs_kfs_rdtsc() <= l->lease_tsc[i])
        {
            return -EAGAIN; 
        }
        else 
        {
            unsigned long * mapped_ring_addr = 
                    sufs_tgroup[l->owner[i]].map_ring_kaddr;

            if (test_bit(ino, mapped_ring_addr))
            {
                tgroup = sufs_kfs_tgid_to_tgroup(tgid);
                if (tgroup && sinode) 
                {
                    vma = tgroup->mount_vma;
                    sufs_do_real_unmap_file(&sufs_sb, ino, sinode->file_type, 
                        sinode->index_offset, sinode, vma, tgroup, tgid);
                }
            }

            return ret;
        }
    }

    if (new_state == SUFS_KFS_READ_OWNED && l->state == SUFS_KFS_READ_OWNED)
    {
        if (l->owner_cnt > SUFS_KFS_LEASE_MAX_OWNER)
            return -ENOSPC;
    }

    return ret;
}


int sufs_kfs_acquire_write_lease(int ino, struct sufs_kfs_lease *l, 
                                 struct sufs_shadow_inode * sinode, int tgid)
{
    int ret = 0;
    unsigned long flags = 0;

    local_irq_save(flags);
    sufs_spin_lock(&(l->lock));

    ret = sufs_kfs_try_acquire(ino, l, sinode, tgid, SUFS_KFS_WRITE_OWNED);

    if (ret == 0)
    {
        sufs_kfs_clean_up_force_info(ino, l);

        l->state = SUFS_KFS_WRITE_OWNED;
        l->owner_cnt = 1;
        l->owner[0] = tgid;
    }

    sufs_spin_unlock(&(l->lock));
    local_irq_restore(flags);

    return ret;
}


static int sufs_kfs_is_acquired_lock(struct sufs_kfs_lease *l, int tgid,
        int *index)
{
    if (l->state == SUFS_KFS_UNOWNED)
        return 0;
    else if (l->state == SUFS_KFS_WRITE_OWNED)
    {
        return (tgid == l->owner[0]);
    }
    /* read owned case */
    else
    {
        int i = 0;
        for (i = 0; i < l->owner_cnt; i++)
        {
            if (tgid == l->owner[i])
            {
                if (index)
                    (*index) = i;
                return 1;
            }
        }

        return 0;
    }
}

int sufs_kfs_release_lease(int ino, struct sufs_kfs_lease *l, int tgid)
{
    unsigned long flags;
    int ret = 0, index = 0;

    local_irq_save(flags);
    sufs_spin_lock(&(l->lock));

    if (l->state == SUFS_KFS_WRITE_OWNED)
    {
        sufs_kfs_clean_up_force_info(ino, l);

        l->state = SUFS_KFS_UNOWNED;
        l->owner_cnt = 0;
        ret = 0;
    }
    else if (l->state == SUFS_KFS_READ_OWNED)
    {
        l->owner[index] = 0;
        sufs_kfs_gc_lease(l);

        if (l->owner_cnt == 0)
        {
            l->state = SUFS_KFS_UNOWNED;
        }

        ret = 0;
    }

    sufs_spin_unlock(&(l->lock));
    local_irq_restore(flags);

    return ret;
}

int sufs_kfs_acquire_read_lease(int ino, struct sufs_kfs_lease *l, 
    struct sufs_shadow_inode * sinode, int tgid)
{

    int ret = 0;
    unsigned long flags = 0;

    local_irq_save(flags);
    sufs_spin_lock(&(l->lock));

    if ((ret = sufs_kfs_try_acquire(ino, l, sinode, tgid, 
        SUFS_KFS_READ_OWNED)) > 0)
    {
        if (l->state == SUFS_KFS_READ_OWNED)
        {
            l->owner[l->owner_cnt] = tgid;
            l->owner_cnt++;
        }
        else
        {
            l->state = SUFS_KFS_READ_OWNED;
            l->owner_cnt = 1;
            l->owner[0] = tgid;
        }

        ret = 0;
    }

    sufs_spin_unlock(&(l->lock));
    local_irq_restore(flags);
    return ret;
}


int sufs_kfs_renew_lease(int ino)
{
    struct sufs_kfs_lease *l = sufs_kfs_get_lease(ino);
    unsigned long flags;
    int ret = 0, index = 0;

    int tgid = sufs_kfs_pid_to_tgid(current->tgid, 0);

    local_irq_save(flags);
    sufs_spin_lock(&(l->lock));

    /* check whether the lease has been acquired by the current trust group */
    if (!sufs_kfs_is_acquired_lock(l, tgid, &index))
    {
        ret = -EINVAL;
        goto out;
    }

out:
    sufs_spin_unlock(&(l->lock));
    local_irq_restore(flags);
    
    return ret;
}

int sufs_kfs_acquire_rename_lease(struct sufs_kfs_lease *l)
{
    int ret = 0;
    unsigned long flags = 0;

    local_irq_save(flags);
    sufs_spin_lock(&(l->lock));

    if (l->owner_cnt != 0 && sufs_kfs_rdtsc() <= l->lease_tsc[0])
    {
        ret = -EAGAIN; 
        goto out; 
    }

    l->state = SUFS_KFS_WRITE_OWNED;
    l->owner_cnt = 1;
    l->lease_tsc[0] = 
        sufs_kfs_rdtsc() + SUFS_LEASE_CYCLES;

out: 
    sufs_spin_unlock(&(l->lock));
    local_irq_restore(flags);
    return ret;
}

int sufs_kfs_release_rename_lease(struct sufs_kfs_lease *l)
{
    unsigned long flags;
    int ret = 0;

    local_irq_save(flags);
    sufs_spin_lock(&(l->lock));

    if (l->state == SUFS_KFS_WRITE_OWNED)
    {
        l->state = SUFS_KFS_UNOWNED;
        l->owner_cnt = 0;
        ret = 0;
    }

    sufs_spin_unlock(&(l->lock));
    local_irq_restore(flags);

    return ret;
}


