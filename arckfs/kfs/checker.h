#ifndef SUFS_KFS_CHECKER_H_
#define SUFS_KFS_CHECKER_H_

#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "../include/checker_config.h"
#include "../include/kfs_config.h"
#include "../include/ioctl.h"

#include "super.h"
#include "simple_ring_buffer.h"

extern struct sufs_checker_ring files_ring, res_ring;


struct sufs_checker_ring
{
    struct sufs_ring_buffer *ring;
};

extern int *sufs_inode_state, *sufs_mapped_state, *sufs_page_state;

static inline void sufs_checker_set_inodes_state(int inode, int num, int state)
{
    int end = inode + num, i = 0;

    if (inode < 0 || inode >= SUFS_MAX_INODE_NUM)
        return;

    if (end < 0 || end >= SUFS_MAX_INODE_NUM)
        return;

    for (i = inode; i < end; i++)
        sufs_inode_state[i] = state;
}


static inline void sufs_checker_set_pages_state(unsigned long block, 
    unsigned long num, int state)
{
    unsigned long end = block + num, i = 0;

    if (block >= (sufs_sb.tot_bytes >> SUFS_PAGE_SHIFT))
        return;

    if (end >= (sufs_sb.tot_bytes >> SUFS_PAGE_SHIFT))
        return;

#if 0
    printk("set pages state: block=%lu, num=%lu, state=%d\n", block, 
        num, state);
#endif

    for (i = block; i < end; i++)
        sufs_page_state[i] = state;
}

static inline int 
sufs_checker_queue_init(struct sufs_checker_ring *q,    
        unsigned long * ring_addr, unsigned long size)
{

    q->ring = sufs_kfs_sr_create(ring_addr,
                                 sizeof(struct sufs_ioctl_checker_entry), 
                                 size);

    return 0;
}

static inline void 
sufs_checker_queue_send(struct sufs_checker_ring *q,
                                    struct sufs_ioctl_checker_entry *e)
{
    int ret = 0, cond_cnt = 0;
    do
    {
        ret = sufs_kfs_sr_send_request(q->ring, e);
        if (ret == -SUFS_RBUFFER_AGAIN)
        {
            cond_cnt++;
            if (cond_cnt >= SUFS_CHECKER_RING_BUFFER_CHECK_COUNT)
            {
                if (need_resched())
                    cond_resched();

                cond_cnt = 0;
            }
        }
    } while (ret == -SUFS_RBUFFER_AGAIN);
}


static inline void sufs_do_send_file_to_checker(struct 
        sufs_ioctl_checker_entry *e)
{
    sufs_checker_queue_send(&files_ring, e);
}

static inline void 
sufs_send_file_to_checker(int ino, char file_type, 
                               unsigned long index_offset, int tgid, 
                               int uid, int gid)
{
    struct sufs_ioctl_checker_entry e = {
        .file_type = file_type, 
        .ino_num = ino,
        .tgid = tgid, 
        .uid = uid, 
        .gid = gid,
        .index_offset = index_offset
    };

    sufs_do_send_file_to_checker(&e);
}


static inline int  
sufs_checker_queue_receive(struct sufs_checker_ring *q,
                                       struct sufs_ioctl_checker_entry *e)
{
    int ret = 0, cond_cnt = 0; 
    long exit_cnt = 0;
    do
    {
        ret = sufs_kfs_sr_receive_request(q->ring, e, 0);
        if (ret == -SUFS_RBUFFER_AGAIN)
        {
            cond_cnt++;
            exit_cnt++;
            if (cond_cnt >= SUFS_CHECKER_RING_BUFFER_CHECK_COUNT)
            {
                if (need_resched())
                    cond_resched();

                cond_cnt = 0;
            }

            if (exit_cnt >= SUFS_CHECKER_RING_BUFFER_EXIT_COUNT)
            {
                printk("Checker: exit due to no request for a long time!\n");
                return -1; 
            }
        }
    } while (ret == -SUFS_RBUFFER_AGAIN);

    return 0;
}

static inline int 
sufs_get_result_from_checker(int ino)
{
    int ret = 0;
    struct sufs_ioctl_checker_entry e;

    while (1)
    {
        ret = sufs_checker_queue_receive(&res_ring, &e);

        if (ret == -1)
        {
            sufs_kfs_checker = 0;
            return 1;
        }

        break;
    }
    return e.ret;
}

int sufs_checker_init(void);
void sufs_checker_fini(void);




int sufs_checker_cmd_add_check_result(unsigned long arg);
int sufs_checker_cmd_get_file_to_check(unsigned long arg);

#endif