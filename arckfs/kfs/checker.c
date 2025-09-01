#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dax.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/memory.h>

#include "../include/config.h"
#include "../include/ioctl.h"
#include "../include/checker_config.h"
#include "checker.h"
#include "inode.h"
#include "mmap.h"
#include "super.h"
#include "tgroup.h"
#include "simple_ring_buffer.h"
#include "ring.h"

int * sufs_inode_state, * sufs_mapped_state, * sufs_page_state;

struct sufs_checker_ring files_ring, res_ring;


int sufs_checker_init(void)
{
    int ret = 0;
    unsigned long kaddr = 0, pages = 0, i = 0;

    if ((ret = sufs_kfs_vmalloc_pages(SUFS_MAX_INODE_NUM * sizeof(int),
                                       (unsigned long **)(&kaddr))) != 0)
        return ret;

    sufs_inode_state = (int *) kaddr;

    if ((ret = sufs_kfs_vmalloc_pages(SUFS_MAX_INODE_NUM * sizeof(int),
                                       (unsigned long **)(&kaddr))) != 0)
        return ret;

    sufs_mapped_state = (int *) kaddr;

    pages = sufs_sb.tot_bytes >> SUFS_PAGE_SHIFT;

    if ((ret = sufs_kfs_vmalloc_pages(pages * sizeof(int),
                                       (unsigned long **)(&kaddr))) != 0)
        return ret;

    sufs_page_state = (int *) kaddr;

    for (i = 0; i < SUFS_MAX_INODE_NUM; i++) 
    {
        sufs_inode_state[i] = SUFS_CHECKER_STATE_FREE;
        sufs_mapped_state[i] = SUFS_MAPPED_STATE_FREE;
    }

    for (i = 0; i < pages; i++) 
    {
        sufs_page_state[i] = SUFS_CHECKER_STATE_FREE;
    }

    return 0;
}

void sufs_checker_fini(void)
{
    unsigned long pages = 0; 

    if (sufs_inode_state != NULL)
    {
        vfree(sufs_inode_state);
    }

    if (sufs_mapped_state != NULL)
    {
        vfree(sufs_mapped_state);
    }

    pages = sufs_sb.tot_bytes >> SUFS_PAGE_SHIFT;

    if (sufs_page_state != NULL)
    {
        vfree(sufs_page_state);
    }

    return; 
}