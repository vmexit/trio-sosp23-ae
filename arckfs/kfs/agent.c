#include <linux/io.h>
#include <linux/kthread.h>

#include "../include/kfs_config.h"
#include "../include/ring_buffer.h"
#include "../include/common_inode.h"
#include "agent.h"
#include "ring.h"
#include "stat.h"
#include "util.h"
#include "mmap.h"
#include "ring_buffer.h"

int sufs_kfs_agent_init = 0;

struct mm_struct * sufs_kfs_mm = NULL;
unsigned long sufs_kfs_counter_addr[SUFS_MAX_THREADS];
struct page * sufs_kfs_counter_pg[SUFS_MAX_THREADS];

unsigned long sufs_kfs_buffer_ring_kaddr[SUFS_MAX_CPU];
struct page * sufs_kfs_buffer_ring_pg[SUFS_MAX_CPU];

struct sufs_kfs_agent_args {
    struct sufs_ring_buffer *ring;
    int idx;
    int pm_node;
    int thread;
};

static struct task_struct *sufs_kfs_agent_tasks[SUFS_KFS_MAX_AGENT];
static struct sufs_kfs_agent_args sufs_kfs_agent_args[SUFS_KFS_MAX_AGENT];

static int sufs_kfs_num_of_agents = 0;


static unsigned long user_virt_addr_to_phy_addr(struct mm_struct *mm,
                        unsigned long uaddr)
{
    /* Mostly copied from mm_find_pmd and slow_virt_to_phys */
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pmd_t pmde;
    pte_t *pte;
    unsigned long phys_page_addr, offset;

    pgd = pgd_offset(mm, uaddr);
    if (!pgd_present(*pgd))
        goto out;

    p4d = p4d_offset(pgd, uaddr);
    if (!p4d_present(*p4d))
        goto out;

    pud = pud_offset(p4d, uaddr);
    if (!pud_present(*pud))
        goto out;

    if (pud_large(*pud))
    {
        phys_page_addr = (phys_addr_t)pud_pfn(*pud) << PAGE_SHIFT;
        offset = uaddr & ~PUD_PAGE_MASK;

       /*
        * This part has not been fully debugged and might be the issue,
        * do a printk here for the moment.
        */
        printk("pud_large!\n");

        return (phys_page_addr | offset);
    }

    pmd = pmd_offset(pud, uaddr);

    pmde = *pmd;
    barrier();

    if (!pmd_present(pmde))
        goto out;

    if (pmd_large(pmde))
    {
        phys_page_addr = (phys_addr_t)pmd_pfn(pmde) << PAGE_SHIFT;
        offset = uaddr & ~PMD_PAGE_MASK;

       /*
        * This part has not been fully debugged and might be the issue,
        * do a printk here for the moment.
        */
        printk("pmd_large!\n");

        return (phys_page_addr | offset);
    }

    pte = pte_offset_kernel(pmd, uaddr);
    if (!pte_present(*pte))
        goto out;

    phys_page_addr = (phys_addr_t)pte_pfn(*pte) << PAGE_SHIFT;
    offset = uaddr & ~PAGE_MASK;

    return (phys_page_addr | offset);

out:
    return 0;
}

static int create_agent_tasks(struct mm_struct *mm, unsigned long uaddr,
                  unsigned long bytes, struct sufs_kfs_agent_tasks *tasks)
{
    unsigned long i = 0;
    long left_bytes = bytes;
    int tasks_index = 0;

    i = uaddr;


    while (i < uaddr + bytes)
    {
        unsigned long phy_addr = user_virt_addr_to_phy_addr(mm, i);
        unsigned long kuaddr = 0, size = 0;

        if (phy_addr == 0)
        {
            goto out;
        }

        size = SUFS_KFS_NEXT_PAGE(i) - i;

        if (size > left_bytes)
            size = left_bytes;

        left_bytes -= size;

        kuaddr = (unsigned long)phys_to_virt(phy_addr);

        if ((tasks_index == 0) ||
            (kuaddr != tasks[tasks_index - 1].kuaddr +
                       tasks[tasks_index - 1].size))
        {
            if (tasks_index >= SUFS_KFS_AGENT_TASK_MAX_SIZE)
            {
                /* This should not happen */
                printk("End: tasks_index overflow!\n");
                goto out;
            }

            tasks[tasks_index].kuaddr = kuaddr;
            tasks[tasks_index].size = size;
            tasks_index++;
        }
        else
        {
            tasks[tasks_index - 1].size += size;
        }


        i += size;
    }


    return tasks_index;

out:
    return -1;
}

static void do_read_request(struct mm_struct *mm, unsigned long offset,
        unsigned long uaddr, unsigned long bytes, int zero, atomic_t * notify_cnt)
{
    int i = 0, tasks_index = 0;

    unsigned long kaddr = sufs_kfs_offset_to_virt_addr(offset);

    struct sufs_kfs_agent_tasks tasks[SUFS_KFS_AGENT_TASK_MAX_SIZE];

    SUFS_KFS_DEFINE_TIMING_VAR(address_translation_time);
    SUFS_KFS_DEFINE_TIMING_VAR(memcpy_time);

    SUFS_KFS_START_TIMING(agent_addr_trans_r_t, address_translation_time);
    tasks_index = create_agent_tasks(mm, uaddr, bytes, tasks);
    SUFS_KFS_END_TIMING(agent_addr_trans_r_t, address_translation_time);

    if (tasks_index <= 0)
        goto out;

    SUFS_KFS_START_TIMING(agent_memcpy_r_t, memcpy_time);
    for (i = 0; i < tasks_index; i++)
    {
        if (zero)
        {
            memset((void *)tasks[i].kuaddr, 0, tasks[i].size);
        }
        else
        {
            memcpy((void *)tasks[i].kuaddr, (void *)kaddr, tasks[i].size);
            kaddr += tasks[i].size;
        }
    }
    SUFS_KFS_END_TIMING(agent_memcpy_r_t, memcpy_time);

out:
    atomic_inc(notify_cnt);
    return;
}

/*
 * There are different ways to flush the cache. Here we follow what
 * pmfs do that.
 *
 * Memset: nt
 * others: clwb
 *
 */
static void do_write_request(struct mm_struct *mm, unsigned long offset,
                 unsigned long uaddr, unsigned long bytes, int zero,
                 int flush_cache, atomic_t * notify_cnt)
{
    int i = 0, tasks_index = 0;

    unsigned long kaddr = sufs_kfs_offset_to_virt_addr(offset);

    unsigned long orig_kaddr = kaddr;

    struct sufs_kfs_agent_tasks tasks[SUFS_KFS_AGENT_TASK_MAX_SIZE];

    SUFS_KFS_DEFINE_TIMING_VAR(address_translation_time);
    SUFS_KFS_DEFINE_TIMING_VAR(memcpy_time);

    if (zero)
    {
        SUFS_KFS_START_TIMING(agent_memcpy_w_t, memcpy_time);

        if (flush_cache)
        {
            sufs_kfs_memset_nt((void *)kaddr, 0, bytes);
        }
        else
        {
            memset((void *)kaddr, 0, bytes);
        }

        SUFS_KFS_END_TIMING(agent_memcpy_w_t, memcpy_time);
        goto out;
    }


    SUFS_KFS_START_TIMING(agent_addr_trans_w_t, address_translation_time);
    tasks_index = create_agent_tasks(mm, uaddr, bytes, tasks);
    SUFS_KFS_END_TIMING(agent_addr_trans_w_t, address_translation_time);

    if (tasks_index <= 0)
        goto out;

    SUFS_KFS_START_TIMING(agent_memcpy_w_t, memcpy_time);
    for (i = 0; i < tasks_index; i++)
    {
        memcpy((void *)kaddr, (void *)tasks[i].kuaddr, tasks[i].size);

        kaddr += tasks[i].size;
    }

    if (flush_cache)
    {
        sufs_kfs_clwb_buffer((void *)orig_kaddr, bytes);
    }

    SUFS_KFS_END_TIMING(agent_memcpy_w_t, memcpy_time);

out:
    atomic_inc(notify_cnt);

    return;
}

static int agent_func(void *arg)
{
    struct sufs_delegation_request request;
    struct sufs_ring_buffer *ring = ((struct sufs_kfs_agent_args *)arg)->ring;
    int pm_node = ((struct sufs_kfs_agent_args *)arg)->pm_node;

    int err = 0;
    unsigned long cond_cnt = 0;

    while (1)
    {
        SUFS_KFS_DEFINE_TIMING_VAR(recv_request_time);
        SUFS_KFS_START_TIMING(agent_receive_request_t, recv_request_time);

try_again:
        err = sufs_kfs_sr_receive_request(ring, &request, 0);

        if (err == -SUFS_RBUFFER_AGAIN)
        {
            cond_cnt++;

            if (cond_cnt >= SUFS_KFS_AGENT_RING_BUFFER_CHECK_COUNT)
            {
                if (kthread_should_stop())
                    break;

                if (need_resched())
                    cond_resched();

                cond_cnt = 0;
            }

            goto try_again;
        }

        if (err)
            break;

        SUFS_KFS_END_TIMING(agent_receive_request_t, recv_request_time);

        if (kthread_should_stop())
            break;

        if (request.type == SUFS_DELE_REQUEST_READ)
        {
            atomic_t * notify_cnt = (atomic_t *)
                       (sufs_kfs_counter_addr[request.notify_idx]
                     + (request.level -1) * (sizeof(struct sufs_notifyer) * SUFS_PM_MAX_INS)
                     + pm_node * sizeof(struct sufs_notifyer));

            do_read_request(sufs_kfs_mm, request.offset,
                    request.uaddr, request.bytes,
                    request.zero, notify_cnt);
        }
        else if (request.type == SUFS_DELE_REQUEST_WRITE)
        {
            atomic_t * notify_cnt = (atomic_t *)
                       (sufs_kfs_counter_addr[request.notify_idx]
                     + (request.level -1) * (sizeof(struct sufs_notifyer) * SUFS_PM_MAX_INS)
                     + pm_node * sizeof(struct sufs_notifyer));

            do_write_request(sufs_kfs_mm, request.offset,
                     request.uaddr, request.bytes,
                     request.zero, request.flush_cache, notify_cnt);
        }
        else if (request.type == SUFS_DELE_REQUEST_KFS_CLEAR)
        {
            atomic_t * notify_cnt = (atomic_t *) request.kidx_ptr;

            do_write_request(NULL, request.offset, 0, request.bytes,
                             request.zero, request.flush_cache, notify_cnt);
        }
        else
        {
            printk("Unknown request type: %d", request.type);
        }

        cond_cnt++;

        if (cond_cnt >= SUFS_KFS_AGENT_REQUEST_CHECK_COUNT)
        {
            if (kthread_should_stop())
                break;

            if (need_resched())
                cond_resched();

            cond_cnt = 0;
        }
    }

    while (!kthread_should_stop())
    {
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
    }

    return err;
}

void sufs_kfs_init_agents(struct sufs_super_block * sufs_sb)
{
    int i = 0, j = 0, cpus_per_socket =0;
    int ret = 0;

    char name[255];
    memset(sufs_kfs_agent_tasks, 0,
            sizeof(struct task_struct *) * SUFS_KFS_MAX_AGENT);

    cpus_per_socket = num_online_cpus() / num_online_nodes();
    sufs_kfs_num_of_agents = sufs_sb->pm_nodes * sufs_kfs_dele_thrds;

    for (i = 0; i < sufs_sb->pm_nodes; i++)
    {
        for (j = 0; j < sufs_kfs_dele_thrds; j++)
        {
            int target_cpu = i * cpus_per_socket + j;
            int index = i * sufs_kfs_dele_thrds + j;

            struct task_struct *task;

            sufs_kfs_agent_args[index].ring = sufs_kfs_ring_buffer[i][j];
            sufs_kfs_agent_args[index].idx = index;
            sufs_kfs_agent_args[index].pm_node = i;
            sufs_kfs_agent_args[index].thread = j;

            sprintf(name, "sufs_kfs_agent_%d_cpu_%d", index, target_cpu);

            task = kthread_create(agent_func, &sufs_kfs_agent_args[index], name);

            if (IS_ERR(task))
            {
                ret = PTR_ERR(task);
                panic("Create task failed with error: %d\n", ret);
            }

            sufs_kfs_agent_tasks[index] = task;

            kthread_bind(sufs_kfs_agent_tasks[index], target_cpu);

            wake_up_process(sufs_kfs_agent_tasks[index]);
        }
    }
}

void sufs_kfs_agents_fini(void)
{
    int i = 0;

    for (i = 0; i < sufs_kfs_num_of_agents; i++)
    {
        if (sufs_kfs_agent_tasks[i])
        {
            int ret;
            if ((ret = kthread_stop(sufs_kfs_agent_tasks[i])))
                printk("kthread_stop task returned error %d\n", ret);
        }
    }
}
