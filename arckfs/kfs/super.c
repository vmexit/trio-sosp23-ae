#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dax.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/memory.h>

#include "../include/kfs_config.h"
#include "../include/common_inode.h"
#include "super.h"
#include "mmap.h"
#include "ring.h"
#include "inode.h"
#include "tgroup.h"
#include "balloc.h"
#include "agent.h"
#include "ring_buffer.h"
#include "checker.h"

struct sufs_super_block sufs_sb;
int sufs_kfs_delegation = 0, sufs_kfs_checker = 0;

void sufs_init_sb(void)
{
    int cpus = num_online_cpus();
    memset(&sufs_sb, 0, sizeof(struct sufs_super_block));

    sufs_sb.sockets = num_online_nodes();
    sufs_sb.cpus_per_socket = cpus / sufs_sb.sockets;

    sufs_sb.dele_ring_per_node = sufs_kfs_dele_thrds;
}

void sufs_sb_update_one_dev(int node, unsigned long virt_addr,
        unsigned long size_in_bytes)
{
    if (sufs_sb.start_virt_addr == 0 || virt_addr < sufs_sb.start_virt_addr)
    {
        sufs_sb.start_virt_addr = virt_addr;
        sufs_sb.head_node = node;
    }

    if (sufs_sb.end_virt_addr ==  0 ||
            virt_addr + size_in_bytes - 1 > sufs_sb.end_virt_addr)
    {
        sufs_sb.end_virt_addr = virt_addr + size_in_bytes - 1;
    }
}

void sufs_sb_update_devs(struct sufs_dev_arr * sufs_dev_arr)
{
    int i = 0;

    sufs_sb.pm_nodes = sufs_dev_arr->num;

    for (i = 0; i < sufs_sb.pm_nodes; i++)
    {
        unsigned long end_vaddr = 0;

        end_vaddr = sufs_dev_arr->start_virt_addr[i] +
                sufs_dev_arr->size_in_bytes[i] - 1;

        sufs_sb.pm_node_info[i].start_block =
                sufs_kfs_virt_addr_to_block(sufs_dev_arr->start_virt_addr[i]);

        sufs_sb.pm_node_info[i].end_block =
                sufs_kfs_virt_addr_to_block(end_vaddr);
    }

}

/* init file system related fields */

/*
 * One page superblock
 * Multiple pages for shadow inode
 * One extra page for root inode
 */
static void sufs_sb_fs_init(int soft)
{
    unsigned long sinode_size = 0;
    int i = 0;

    sinode_size = SUFS_MAX_INODE_NUM * sizeof(struct sufs_shadow_inode);

    sufs_sb.sinode_start = (struct sufs_shadow_inode *)
                               (sufs_sb.start_virt_addr + SUFS_SUPER_PAGE_SIZE);

    if (!soft)
    {
        for (i = 0; i < SUFS_MAX_INODE_NUM; i++)
        {
            sufs_sb.sinode_start[i].file_type = SUFS_FILE_TYPE_NONE;
            sufs_kfs_init_lease(&sufs_sb.sinode_start[i].lease);
        }
    }

    sufs_sb.head_reserved_blocks = (sinode_size >> PAGE_SHIFT) + 2;

    sufs_init_inode_free_list(&sufs_sb);

    sufs_init_block_free_list(&sufs_sb, 0);
}

long sufs_reserve_vmas_for_pm(void)
{
    struct file *file;

    long ret = 0;

    file = filp_open(SUFS_DEV_PATH, O_RDWR, 0);

    if (IS_ERR(file))
    {
        ret = PTR_ERR(file);
        printk("Open: %s failed with error : %d\n", SUFS_DEV_PATH, (int)ret);

        return ret;
    }

   
    ret = vm_mmap(file, SUFS_MOUNT_ADDR, sufs_sb.tot_bytes,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, 0);

    filp_close(file, NULL);

    if (IS_ERR_VALUE(ret))
        return ret;

    return ret;
}

long sufs_mount(void)
{
    long ret = 0;

    struct sufs_tgroup * tgroup = NULL;

    tgroup = sufs_kfs_pid_to_tgroup(current->tgid, 1);

    if (!tgroup)
    {
        printk("Cannot allocate tgroup during mount!\n");
        return -ENOMEM;
    }

    ret = sufs_reserve_vmas_for_pm();

    if (IS_ERR_VALUE(ret))
        return ret;

    ret = sufs_kfs_create_ring(tgroup);

    if (IS_ERR_VALUE(ret))
    {
        vm_munmap(SUFS_MOUNT_ADDR, sufs_sb.tot_bytes);
        return ret;
    }

    tgroup->mount_vma = find_vma(current->mm, SUFS_MOUNT_ADDR);
    if (tgroup->mount_vma == NULL)
    {
        printk("Cannot find the mount vma!\n");
        return -ENOMEM;
    }

    sufs_kfs_mm = current->mm;

    return ret;
}

int sufs_umount(unsigned long addr)
{
    int ret = 0;
    struct sufs_tgroup * tgroup = NULL;

    ret = vm_munmap(addr, sufs_sb.tot_bytes);

    tgroup = sufs_kfs_pid_to_tgroup(current->tgid, 1);

    if (!tgroup)
    {
        printk("Cannot find the tgroup with pid during umount: %d\n",
                current->tgid);

        return -ENOMEM;
    }

    sufs_kfs_delete_ring(tgroup);

    return ret;
}

int sufs_checker_map(unsigned long arg)
{
    long ret = 0;
    struct vm_area_struct *vma = NULL;
    pgprot_t prop;
    struct sufs_ioctl_checker_map_entry entry;

    unsigned long inode_state_size = SUFS_INODE_STATE_SIZE;
    unsigned long mapped_state_size = SUFS_MAPPED_STATE_SIZE;
    unsigned long page_state_size = (sufs_sb.tot_bytes >> SUFS_PAGE_SHIFT) 
                                            * sizeof(int);

    unsigned long * ring_kaddr = NULL;


    ret = sufs_reserve_vmas_for_pm();
    if (IS_ERR_VALUE(ret))
        return ret;

    vma = find_vma(current->mm, SUFS_MOUNT_ADDR);

    if (!vma)
    {
        printk("Cannot find the mount vma!\n");
        return -ENOMEM;
    }

    prop = vm_get_page_prot(VM_READ);
    sufs_map_pages(vma, SUFS_MOUNT_ADDR, sufs_kfs_offset_to_pfn(0), prop,
                   sufs_sb.tot_bytes >> PAGE_SHIFT);

    ret = vm_mmap(NULL, SUFS_STATE_ADDR, inode_state_size + 
        mapped_state_size + page_state_size,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, 0);

    if (IS_ERR_VALUE(ret))
        return ret;

    vma = find_vma(current->mm, SUFS_STATE_ADDR);

    if (!vma)
    {
        printk("Cannot find the mount vma!\n");
        return -ENOMEM;
    }

    /*  Makes vmf_insert_pfn_prot happy */
    /*  TODO: validate whether VM_PFNMAP has any side effect or not */
    vma->vm_flags |= VM_PFNMAP;

    ret = sufs_kfs_mmap_vmalloc_pages_to_user(
            (unsigned long) sufs_inode_state,
            SUFS_STATE_ADDR, inode_state_size, vma, 1);

    if (ret < 0)
    {
        printk("sufs_kfs_mmap_vmalloc_pages_to_user (inode_state) failed\n");
        goto err_unmap; 
    }

    ret = sufs_kfs_mmap_vmalloc_pages_to_user(
            (unsigned long) sufs_page_state, 
            SUFS_STATE_ADDR + inode_state_size, 
            page_state_size, vma, 1);
    
    if (ret < 0)
    {
        printk("sufs_kfs_mmap_vmalloc_pages_to_user (page_state) failed\n");
        goto err_unmap; 
    }

    ret = vm_mmap(NULL, SUFS_CHECKER_FILE_RING_ADDR, 
                  SUFS_CHECKER_FILE_RING_SIZE + SUFS_CHECKER_RES_RING_SIZE,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, 0);

    if (IS_ERR_VALUE(ret))
        return ret;

    vma = find_vma(current->mm, SUFS_CHECKER_FILE_RING_ADDR);

    if (!vma)
    {
        printk("Cannot find the mount vma!\n");
        return -ENOMEM;
    }

    /*  Makes vmf_insert_pfn_prot happy */
    /*  TODO: validate whether VM_PFNMAP has any side effect or not */
    vma->vm_flags |= VM_PFNMAP;
    
    ret = sufs_kfs_mmap_ring_vmalloc(SUFS_CHECKER_FILE_RING_ADDR,
            SUFS_CHECKER_FILE_RING_SIZE, vma, 1, &(ring_kaddr));

    if (ret < 0)
    {
        printk("sufs_kfs_mmap_ring_vmalloc (file_ring) failed\n");
        goto err_unmap;
    }

    sufs_checker_queue_init(&files_ring, 
            ring_kaddr, SUFS_CHECKER_FILE_RING_SIZE);

    ret = sufs_kfs_mmap_ring_vmalloc(SUFS_CHECKER_RES_RING_ADDR,
            SUFS_CHECKER_RES_RING_SIZE, vma, 1, &(ring_kaddr));

    if (ret < 0)
    {
        printk("sufs_kfs_mmap_ring_vmalloc (res_ring) failed\n");
        goto err_unmap;
    }

    sufs_checker_queue_init(&res_ring, ring_kaddr, SUFS_CHECKER_RES_RING_SIZE);
    
    
    entry.max_block = sufs_kfs_virt_addr_to_block(sufs_sb.end_virt_addr);

    if (copy_to_user((void *)arg, &entry, 
                            sizeof(struct sufs_ioctl_checker_map_entry)))
        return -EFAULT; 

    sufs_kfs_checker = 1;

    return ret;

err_unmap:
    printk("vm_mmap failed!\n");
    vm_munmap(SUFS_RING_ADDR, inode_state_size + page_state_size);
    return -ENOMEM;
}

long sufs_debug_read()
{
    return 0;
}

static void sufs_init_root_inode(void)
{
    unsigned long data_block = sufs_sb.head_reserved_blocks - 1;
    unsigned long vaddr = sufs_kfs_block_to_virt_addr(data_block);

    memset((void *) vaddr, 0, PAGE_SIZE);

    sufs_kfs_set_inode(SUFS_ROOT_INODE, SUFS_FILE_TYPE_DIR,
            SUFS_ROOT_PERM, 0, 0, 
            sufs_kfs_block_to_offset(data_block), SUFS_ROOT_INODE);

    sufs_inode_state[SUFS_ROOT_INODE] = SUFS_CHECKER_STATE_EXIST;
}


/* Format the file system */
int sufs_fs_init(int soft)
{
    int ret = 0;

    if ((ret = sufs_kfs_init_tgroup()) != 0)
        goto fail;

    if ((ret = sufs_init_rangenode_cache()) != 0)
        goto fail_rangenode;

    if ((ret = sufs_alloc_inode_free_list(&sufs_sb)) != 0)
        goto fail_inode_free_list;

    if ((ret = sufs_alloc_block_free_lists(&sufs_sb)) != 0)
        goto fail_block_free_lists;

    if ((ret = sufs_checker_init()) != 0)
        goto fail_checker_init;

    sufs_sb_fs_init(soft);

    if (!soft)
    {
        sufs_init_root_inode();
    }

    if (sufs_kfs_agent_init)
    {
        sufs_kfs_agents_fini();
        sufs_kfs_fini_ring_buffers(sufs_sb.pm_nodes);
    }

    sufs_kfs_init_ring_buffers(sufs_sb.pm_nodes);

    sufs_kfs_init_agents(&sufs_sb);

    sufs_kfs_rename_lease_init();
    
    sufs_kfs_agent_init = 1;

    return 0;

fail_block_free_lists:
    sufs_free_inode_free_list(&sufs_sb);
fail_inode_free_list:
    sufs_free_rangenode_cache();

fail_rangenode:
    sufs_kfs_fini_tgroup();

fail_checker_init:
    sufs_checker_fini();

fail:
    return ret;

}

void sufs_fs_fini(void)
{
    if (sufs_kfs_agent_init)
    {
        sufs_kfs_agents_fini();

        sufs_kfs_fini_ring_buffers(sufs_sb.pm_nodes);
    }

    sufs_delete_block_free_lists(&sufs_sb);

    sufs_free_inode_free_list(&sufs_sb);

    sufs_free_rangenode_cache();

    sufs_kfs_fini_tgroup();

    sufs_checker_fini();
    
    return;
}

