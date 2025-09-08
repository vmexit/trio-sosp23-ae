#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/vmalloc.h>

#include "../include/kfs_config.h"
#include "tgroup.h"


char * sufs_kfs_pid_to_tgroups = NULL;

spinlock_t sufs_tgroup_lock;

struct sufs_tgroup * sufs_tgroup = NULL;


static inline int can_modify_tgroup(void)
{
    const struct cred * cred = NULL;

    cred = current_cred();

    return (cred->uid.val == 0);
}

int sufs_kfs_init_tgroup(void)
{
    sufs_tgroup = vzalloc(sizeof(struct sufs_tgroup) * SUFS_MAX_TGROUP);

    if (!sufs_tgroup)
    {
        printk("allocating sufs_tgroup fail with elem size: %ld and num: %d",
                sizeof(struct sufs_tgroup), SUFS_MAX_TGROUP);
        return -ENOMEM;
    }

    sufs_kfs_pid_to_tgroups = vzalloc(sizeof(char) * SUFS_MAX_PROCESS);
    if (!sufs_kfs_pid_to_tgroups)
    {
        printk("allocating sufs_kfs_pid_to_tgroups fail with elem size: %ld and "
                "num: %d",
                sizeof(char), SUFS_MAX_PROCESS);

        vfree(sufs_tgroup);

        return -ENOMEM;
    }

    spin_lock_init(&sufs_tgroup_lock);

    return 0;
}

void sufs_kfs_fini_tgroup(void)
{
    if (sufs_tgroup)
        vfree(sufs_tgroup);

    sufs_tgroup = NULL;

    if (sufs_kfs_pid_to_tgroups)
        vfree(sufs_kfs_pid_to_tgroups);

    sufs_kfs_pid_to_tgroups = NULL;
}


static int __sufs_kfs_alloc_tgroup(int pid)
{
    int i = 0;

    for (i = SUFS_MIN_TGROUP; i < SUFS_MAX_TGROUP; i++)
    {
        if (!sufs_tgroup[i].used)
            goto found;
    }

    printk("Reach the maximum number of tgroup!\n");
    return -ENOSPC;

found:

    memset(&(sufs_tgroup[i]), 0, sizeof(struct sufs_tgroup));
    sufs_tgroup[i].used = 1;

    if (pid)
    {
        sufs_tgroup[i].max_index = 1;
        sufs_tgroup[i].pid[0] = pid;
        sufs_kfs_pid_to_tgroups[pid] = i;
    }

    return i;
}

/* alloc an empty tgroup for the user */
int sufs_kfs_alloc_tgroup(void)
{
    unsigned long flag = 0;
    int ret = 0;
    if (!can_modify_tgroup())
        return -EPERM;

    spin_lock_irqsave(&sufs_tgroup_lock, flag);

    ret = __sufs_kfs_alloc_tgroup(0);

    spin_unlock_irqrestore(&sufs_tgroup_lock, flag);

    return ret;
}

/* invoked when the sufs_tgroup_lock is held */
static int __sufs_kfs_free_tgroup(int tgid)
{
    int i = 0;
    struct sufs_tgroup * tgroup = NULL;

    if (tgid >= SUFS_MAX_TGROUP)
        return -EINVAL;

    tgroup = &(sufs_tgroup[tgid]);

    if (!(tgroup->used))
        return -EINVAL;

    for (i = 0; i < tgroup->max_index; i++)
    {
        unsigned int pid = tgroup->pid[i];
        /* clear the index array */
        if (pid != 0)
        {
            sufs_kfs_pid_to_tgroups[pid] = 0;
        }
    }

    memset(tgroup, 0, sizeof(struct sufs_tgroup));

    return 0;
}

int sufs_kfs_free_tgroup(int tgid)
{
    int ret = 0;
    unsigned long flag = 0;

    if (!can_modify_tgroup())
        return -EPERM;

    spin_lock_irqsave(&sufs_tgroup_lock, flag);

    ret = __sufs_kfs_free_tgroup(tgid);

    spin_unlock_irqrestore(&sufs_tgroup_lock, flag);

    return ret;
}

static void sufs_kfs_gc_tgroup(struct sufs_tgroup * tgroup)
{
    int i = 0;

    /* XXX: The below array might overflow with a
      large SUFS_MAX_PROCESS_PER_TGROUP */

    unsigned int spid[SUFS_MAX_PROCESS_PER_TGROUP];
    int smax_index = 0;

    for (i = 0; i < tgroup->max_index; i++)
    {
        if (tgroup->pid[i] != 0)
        {
            spid[smax_index] = tgroup->pid[i];
            smax_index++;
        }
    }

    for (i = 0; i < smax_index; i++)
    {
        tgroup->pid[i] = spid[i];
    }

    tgroup->max_index = smax_index;
}

int sufs_kfs_tgroup_add_process(int tgid, int pid)
{
    struct sufs_tgroup * tgroup = NULL;

    if (!can_modify_tgroup())
        return -EPERM;

    if (tgid >= SUFS_MAX_TGROUP)
        return -EINVAL;

    tgroup = &(sufs_tgroup[tgid]);

    if (!(tgroup->used))
        return -EINVAL;


    if (tgroup->max_index == SUFS_MAX_PROCESS_PER_TGROUP)
    {
        sufs_kfs_gc_tgroup(tgroup);
        if (tgroup->max_index == SUFS_MAX_PROCESS_PER_TGROUP)
        {
            printk("Reach the maximum process within one tgroup!\n");
            return -ENOSPC;
        }
    }

    sufs_kfs_pid_to_tgroups[pid] = tgid;
    tgroup->pid[tgroup->max_index] = pid;
    tgroup->max_index++;

    return 0;
}

int sufs_kfs_tgroup_remove_process(int tgid, int pid)
{
    int i = 0;
    struct sufs_tgroup * tgroup = NULL;

    if (!can_modify_tgroup())
        return -EPERM;

    if (tgid >= SUFS_MAX_TGROUP)
        return -EINVAL;

    tgroup = &(sufs_tgroup[tgid]);

    if (!(tgroup->used))
        return -EINVAL;

    for (i = 0; i < tgroup->max_index; i++)
    {
        if (tgroup->pid[i] == pid)
            goto found;
    }

    printk("pid: %d not found in tgid: %d\n", pid, tgid);
    return -EINVAL;

found:
    tgroup->pid[i] = 0;
    sufs_kfs_pid_to_tgroups[pid] = 0;

    if (i == tgroup->max_index - 1)
        tgroup->max_index--;

    return 0;
}

int sufs_kfs_pid_to_tgid_alloc(int pid)
{
    unsigned long flag = 0;
    int ret = 0;

    spin_lock_irqsave(&sufs_tgroup_lock, flag);

    ret = sufs_kfs_pid_to_tgroups[pid];

    if (ret != 0)
        goto out;

    ret = __sufs_kfs_alloc_tgroup(pid);

out:
    spin_unlock_irqrestore(&sufs_tgroup_lock, flag);

    return ret;

}

