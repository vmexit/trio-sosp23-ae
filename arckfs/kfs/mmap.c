#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/cred.h>

#include "../include/kfs_config.h"
#include "../include/ioctl.h"
#include "../include/common_inode.h"

#include "checker.h"
#include "mmap.h"
#include "tgroup.h"
#include "file.h"
#include "inode.h"
#include "util.h"

static vm_fault_t sufs_page_fault(struct vm_fault *vmf)
{
    /* Should not trigger page fault */
    printk("Enter sufs page fault: address: %lx!\n",
            vmf->address);
    return VM_FAULT_SIGBUS;
}

static const struct vm_operations_struct sufs_vm_ops = { .fault =
        sufs_page_fault, };

int sufs_kfs_mmap(struct file *filp, struct vm_area_struct *vma)
{
    vma->vm_ops = &sufs_vm_ops;

    /*  Makes vmf_insert_pfn_prot happy */
    /*  TODO: validate whether VM_PFNMAP has any side effect or not */
    vma->vm_flags |= VM_PFNMAP;

    return 0;
}

/* Simplified permission check, does not consider many factors ... */
static unsigned long sufs_gen_access_perm(struct sufs_shadow_inode *sinode,
        int write)
{
    const struct cred *cred = current_cred();

    unsigned long ret = 0;

    /* owner */
    if (sinode->uid == cred->uid.val)
    {
        if (sinode->mode & S_IRUSR)
        {
            ret |= VM_READ;
            if (write && (sinode->mode & S_IWUSR))
                ret |= VM_WRITE;
        }
    }
    /* group */
    else if (sinode->gid == cred->gid.val)
    {
        if (sinode->mode & S_IRGRP)
        {
            ret |= VM_READ;
            if (write && (sinode->mode & S_IWGRP))
                ret |= VM_WRITE;
        }
    }
    /* others */
    else
    {
        if (sinode->mode & S_IROTH)
        {
            ret |= VM_READ;
            if (write && (sinode->mode & S_IWOTH))
                ret |= VM_WRITE;
        }
    }

    if (ret == 0)
        return ret;
    else
        return (ret | VM_SHARED);
}

static void sufs_kfs_dir_delete_sinode_one_file_block(unsigned long offset, 
        int ino_curr)
{
    struct sufs_dir_entry *dir = (struct sufs_dir_entry *)
        sufs_kfs_offset_to_virt_addr(offset);

    while (dir->name_len != 0)
    {

        if (dir->ino_num != SUFS_INODE_TOMBSTONE)
        {
            struct sufs_shadow_inode * si = sufs_find_sinode(dir->ino_num);

            if (si->parent == ino_curr)
            {
                sufs_inode_state[dir->ino_num] = SUFS_CHECKER_STATE_FREE;

                sufs_kfs_set_inode(dir->ino_num, SUFS_FILE_TYPE_NONE,
                                    0, 0, 0, 0, 0);
            }
        }


        dir = (struct sufs_dir_entry *)((unsigned long)dir + dir->rec_len);

        if (SUFS_KFS_FILE_BLOCK_OFFSET(dir) == 0)
            break;
    }

    return;
}

static void sufs_kfs_dir_dir_print_page_info(unsigned long offset)
{
    struct sufs_dir_entry *dir = (struct sufs_dir_entry *)
        sufs_kfs_offset_to_virt_addr(offset);

    while (dir->name_len != 0)
    {
        unsigned short rec_len = dir->rec_len;

        printk("ino_num: %d, file_type: %d, offset: %lx\n", 
            dir->ino_num, dir->inode.file_type, dir->inode.offset);

        dir = (struct sufs_dir_entry *)((unsigned long)dir + rec_len);

        if (SUFS_KFS_FILE_BLOCK_OFFSET(dir) == 0)
            break;
    }

    return;

}

void sufs_kfs_dir_print_info(unsigned long index_offset)
{
    struct sufs_fidx_entry * idx = NULL;

    if (index_offset == 0)
        return;

    idx = (struct sufs_fidx_entry * )
                    sufs_kfs_offset_to_virt_addr(index_offset);

    while (idx->offset != 0)
    {
        if (likely(sufs_is_norm_fidex(idx)))
        {
            printk("offset is %lx\n", idx->offset); 
            sufs_kfs_dir_dir_print_page_info(idx->offset);
            idx++;
        }
        else
        {
            idx = (struct sufs_fidx_entry*)
                    sufs_kfs_offset_to_virt_addr(idx->offset);
        }
    }

    return;
}



static void sufs_kfs_dir_update_sinode_one_file_block(unsigned long offset,
                                                      int set_sinode, 
                                                      int state, int parent)
{
    struct sufs_dir_entry *dir = (struct sufs_dir_entry *)
        sufs_kfs_offset_to_virt_addr(offset);

    while (dir->name_len != 0)
    {
        unsigned short rec_len = dir->rec_len;

        if (dir->ino_num != SUFS_INODE_TOMBSTONE)
        {
            sufs_inode_state[dir->ino_num] = state;
#if 0
            printk("set sufs_inode_state[%d]: %d\n", dir->ino_num, state);
#endif
            if (set_sinode)
            {
                sufs_kfs_set_inode(dir->ino_num, dir->inode.file_type,
                                   dir->inode.mode, dir->inode.uid, 
                                   dir->inode.gid, dir->inode.offset, parent);
            }
        }

        dir = (struct sufs_dir_entry *)((unsigned long)dir + rec_len);

        if (SUFS_KFS_FILE_BLOCK_OFFSET(dir) == 0)
            break;
    }

    return;
}

void sufs_map_pages(struct vm_area_struct * vma,
        unsigned long vaddr, unsigned long pfn, pgprot_t prop, long count)
{
    vm_fault_t rc;

    long i = 0;

    for (i = 0; i < count; i++)
    {
        if ((rc = vmf_insert_pfn_prot(vma, vaddr, pfn, prop)) != VM_FAULT_NOPAGE)
        {
            printk("insert pfn root failed with vaddr: %lx, pfn: %lx, rc: %x\n",
                    vaddr, pfn, rc);
        }

        vaddr += PAGE_SIZE;
        pfn++;
    }
}


static long sufs_map_file_pages(struct sufs_super_block * sb,
        struct sufs_shadow_inode * sinode, struct vm_area_struct * vma,
        pgprot_t prop, int tgid, int parent)
{
    struct sufs_fidx_entry * idx = NULL;

    unsigned long offset = 0;

    if (sinode->index_offset == 0)
        return 0;

    idx = (struct sufs_fidx_entry * )
                    sufs_kfs_offset_to_virt_addr(sinode->index_offset);

    /* map the first index page */
    offset = sufs_kfs_virt_addr_to_offset((unsigned long) idx);

    sufs_page_state[sufs_kfs_offset_to_block(offset)] = tgid;

    sufs_map_pages(vma, SUFS_MOUNT_ADDR + offset,
            sufs_kfs_offset_to_pfn(offset), prop, 1);

    while (idx->offset != 0)
    {
        if (likely(sufs_is_norm_fidex(idx)))
        {
            /* map the data pages */
            offset = idx->offset;

            if (sinode->file_type == SUFS_FILE_TYPE_DIR)
            {
                sufs_kfs_dir_update_sinode_one_file_block(offset, 0, 
                    tgid, parent);
            }

            sufs_map_pages(vma, SUFS_MOUNT_ADDR + offset,
                    sufs_kfs_offset_to_pfn(offset), prop, SUFS_FILE_BLOCK_PAGE_CNT);
            
            sufs_checker_set_pages_state(sufs_kfs_offset_to_block(offset),
                                         SUFS_FILE_BLOCK_PAGE_CNT, tgid);
            
            idx++;
        }
        else
        {
            /* map the index pages */
            idx = (struct sufs_fidx_entry*)
                    sufs_kfs_offset_to_virt_addr(idx->offset);

            offset = sufs_kfs_virt_addr_to_offset((unsigned long) idx);


            sufs_map_pages(vma, SUFS_MOUNT_ADDR + offset,
                    sufs_kfs_offset_to_pfn(offset), prop, 1);
        
            sufs_page_state[sufs_kfs_offset_to_block(offset)] = tgid;

        }
    }

    return 0;
}


static int sufs_kfs_backup_file_page(unsigned long offset_src, 
                                   unsigned long * offset_new, int num_blocks)
{
    int pm_node = 0, ret = 0; 
    unsigned long block_nr = 0;

    pm_node = sufs_block_to_pm_node(&sufs_sb, 
                    sufs_kfs_offset_to_block(offset_src));

    ret = sufs_new_blocks(&sufs_sb, &block_nr, num_blocks, 0, 
        smp_processor_id(), pm_node);

    if (ret < 0)
        return ret;

    memcpy((void *) sufs_kfs_block_to_virt_addr(block_nr),
            (void *) sufs_kfs_offset_to_virt_addr(offset_src),
           num_blocks * SUFS_PAGE_SIZE);

    if (offset_new)
    {
        (*offset_new) = sufs_kfs_block_to_offset(block_nr);
    }

    return 0; 
}

static int sufs_kfs_backup_file_metadata(struct sufs_shadow_inode * sinode)
{
    struct sufs_fidx_entry *idx = NULL, * sidx = NULL;
    int ret = 0;

    unsigned long offset = 0;

    if (sinode->index_offset == 0)
        return 0;

    /* map the first index page */
    if ((ret = sufs_kfs_backup_file_page(sinode->index_offset, 
                                       &(sinode->shadow_index_offset), 1)) != 0)
        return ret;

    idx = (struct sufs_fidx_entry *)
        sufs_kfs_offset_to_virt_addr(sinode->index_offset);

    sidx = (struct sufs_fidx_entry *)
        sufs_kfs_offset_to_virt_addr(sinode->shadow_index_offset);
    
    while (idx->offset != 0)
    {
        if (likely(sufs_is_norm_fidex(idx)))
        {
            /* copy the content of a directory */
            if (sinode->file_type == SUFS_FILE_TYPE_DIR)
            {
                sufs_kfs_backup_file_page(idx->offset, &(sidx->offset), 
                        SUFS_FILE_BLOCK_PAGE_CNT);
            }

            idx++;
            sidx++; 
        }
        else
        {
            /* map the next index page*/
            idx = (struct sufs_fidx_entry *)
                sufs_kfs_offset_to_virt_addr(idx->offset);

            offset = sufs_kfs_virt_addr_to_offset((unsigned long)idx);

            sufs_kfs_backup_file_page(offset, &(sidx->offset), 1);

            sidx = (struct sufs_fidx_entry *)
                sufs_kfs_offset_to_virt_addr(sidx->offset);
        }
    }

    return 0;
}

static void sufs_kfs_free_backup_metadata(int file_type, 
        unsigned long init_offset, int ino_curr)
{
    struct sufs_fidx_entry *idx = NULL;

    unsigned long offset = 0;

    if (init_offset == 0)
        return;

    idx = (struct sufs_fidx_entry *)
        sufs_kfs_offset_to_virt_addr(init_offset);

    while (idx->offset != 0)
    {
        if (likely(sufs_is_norm_fidex(idx)))
        {
            if (file_type == SUFS_FILE_TYPE_DIR)
            {
                offset = idx->offset;
                sufs_kfs_dir_delete_sinode_one_file_block(offset, ino_curr);
            }

            idx++;
        }
        else
        {
            idx = (struct sufs_fidx_entry *)
                sufs_kfs_offset_to_virt_addr(idx->offset);
        }
    }

    return;
}

/*
 * write != 0, mmaped as read and write,
 * otherwise, mmaped as read
 */
static long sufs_do_mmap_file(struct sufs_super_block * sb, int ino,
        int writable, long * index_offset)
{
    struct sufs_tgroup *tgroup = NULL;

    struct vm_area_struct *vma = NULL;

    struct sufs_shadow_inode *sinode = NULL;

    struct sufs_kfs_lease * lease = NULL;

    long ret = 0;

    unsigned long perm = 0;
    int tgid = 0;

    /* This is quite stupid */
    pgprot_t prop;

    tgid = sufs_kfs_pid_to_tgid(current->tgid, 0);

    tgroup = sufs_kfs_tgid_to_tgroup(tgid);

    if (tgroup == NULL)
    {
        printk("Cannot find the tgroup with pid :%d\n", current->tgid);
        return -ENODEV;
    }

    vma = tgroup->mount_vma;

    if (vma == NULL)
    {
        printk("Cannot find the mapped vma\n");
        return -ENODEV;
    }

    sinode = sufs_find_sinode(ino);

    if (sinode == NULL)
    {
        printk("Cannot find sinode with ino %d\n", ino);
        return -EINVAL;
    }

    if (sinode->file_type == SUFS_FILE_TYPE_NONE)
    {
        printk("Cannot map an unexisting sinode with ino %d\n", ino);
        return -EINVAL;    
    }

    lease = &(sinode->lease);

    if (lease == NULL)
    {
        printk("Sinode with empty lease %d\n", ino);
        return -EINVAL;
    }

    perm = sufs_gen_access_perm(sinode, writable);

    if (perm == 0)
    {
        printk("Cannot access file with ino: %d, uid: %d, gid: %d, mode: %d\n",
                ino, sinode->uid, sinode->gid, sinode->mode);
        return -EACCES;
    }

    if (writable && sufs_kfs_checker)
    {
        ret = sufs_kfs_backup_file_metadata(sinode);

        if (ret < 0)
            return ret;
    }

    if (writable)
    {
        ret = sufs_kfs_acquire_write_lease(ino, lease, sinode, tgid);
    }
    else
    {
        ret = sufs_kfs_acquire_read_lease(ino, lease, sinode, tgid);
    }

    if (ret < 0)
    {
        if (ret != -EAGAIN)
        {
            printk("Cannot acquire the lease with ino: %d!\n", ino);
        }
        return ret;
    }

    prop = vm_get_page_prot(perm);

    ret = sufs_map_file_pages(sb, sinode, vma, prop, tgid, ino);

    // After `sufs_map_file_pages()` completes, allow other process to acquire the lease.
    lease->lease_tsc[0] = 0;

    /* Upon successful map, set the ring and index offset*/
    if (ret == 0)
    {
        set_bit(ino, tgroup->map_ring_kaddr);
        if (index_offset)
            *(index_offset) = sinode->index_offset;

        sufs_mapped_state[ino] = tgid;
    }

    return ret;
}

long sufs_mmap_file(unsigned long arg)
{
    long ret = 0;
    struct sufs_ioctl_map_entry entry;

    if (copy_from_user(&entry, (void*) arg,
            sizeof(struct sufs_ioctl_map_entry)))
        return -EFAULT;

    ret = sufs_do_mmap_file(&sufs_sb, entry.inode, entry.perm,
            &entry.index_offset);
    
    if (ret == -EAGAIN)
    {
        entry.again = 1; 
    }
    else
    {
        entry.again = 0;
    }

    if (ret == 0 || ret == -EAGAIN)
    {
        if (copy_to_user((void *) arg, &entry,
                sizeof(struct sufs_ioctl_map_entry)))
            return -EFAULT;
    }

    return ret;
}

static unsigned long sufs_unmap_file_pages(struct sufs_super_block * sb,
        unsigned long index_offset, struct vm_area_struct *vma, int tgid)
{
    struct sufs_fidx_entry *idx = NULL;

    unsigned long offset = 0;

    if (index_offset == 0)
        return 0;

    if (!sufs_kfs_is_offset_valid(index_offset, tgid))
    {
        return -EINVAL;
    }

    idx = (struct sufs_fidx_entry*) sufs_kfs_offset_to_virt_addr(index_offset);

    /* remove the file page mapping */
    offset = sufs_kfs_virt_addr_to_offset((unsigned long) idx);
    zap_vma_ptes(vma, SUFS_MOUNT_ADDR + offset, PAGE_SIZE);

    sufs_page_state[sufs_kfs_offset_to_block(offset)] 
        = SUFS_CHECKER_STATE_EXIST;

    while (idx->offset != 0)
    {
        if (likely(sufs_is_norm_fidex(idx)))
        {
            offset = idx->offset;
            if (!sufs_kfs_is_offset_valid(offset, tgid))
            {
                return -EINVAL;
            }

            /* remove the normal page mapping */
            zap_vma_ptes(vma, SUFS_MOUNT_ADDR + offset, SUFS_FILE_BLOCK_SIZE);
            idx++;

            sufs_checker_set_pages_state(sufs_kfs_offset_to_block(offset),
                                         SUFS_FILE_BLOCK_PAGE_CNT,
                                         SUFS_CHECKER_STATE_EXIST);

        }
        else
        {
            idx = (struct sufs_fidx_entry*)
                    sufs_kfs_offset_to_virt_addr(idx->offset);

            offset = sufs_kfs_virt_addr_to_offset((unsigned long) idx);
            if (!sufs_kfs_is_offset_valid(offset, tgid))
            {
                return -EINVAL;
            }

            /* remove the file page mapping */
            zap_vma_ptes(vma, SUFS_MOUNT_ADDR + offset, PAGE_SIZE);

            sufs_page_state[sufs_kfs_offset_to_block(offset)] 
                = SUFS_CHECKER_STATE_EXIST;
        }
    }

    return 0;
}


static void sufs_kfs_dir_update_sinode(struct sufs_shadow_inode * sinode, 
    int parent)
{
    struct sufs_fidx_entry *idx = NULL;

    if (sinode->index_offset == 0)
        return;

    idx = (struct sufs_fidx_entry *)
            sufs_kfs_offset_to_virt_addr(sinode->index_offset);

    while (idx->offset != 0)
    {
        if (likely(sufs_is_norm_fidex(idx)))
        {
            sufs_kfs_dir_update_sinode_one_file_block(idx->offset, 
                    1, SUFS_CHECKER_STATE_EXIST, parent);
            idx++;
        }
        else
        {
            idx = (struct sufs_fidx_entry*) sufs_kfs_offset_to_virt_addr(
                    idx->offset);
        }
    }

    return;
}

long sufs_do_real_unmap_file(struct sufs_super_block *sb, 
        int ino, char file_type, unsigned long index_offset, 
        struct sufs_shadow_inode *sinode, struct vm_area_struct *vma,  
        struct sufs_tgroup *tgroup, int tgid)
{
    long ret = 0;
    if ( (ret = sufs_unmap_file_pages(sb, sinode->index_offset, vma, tgid)) < 0)
    {
    }

    clear_bit(ino, tgroup->map_ring_kaddr);
    clear_bit(ino, tgroup->wanted_ring_kaddr);
    sufs_mapped_state[ino] = SUFS_MAPPED_STATE_FREE;

    if (sufs_kfs_checker)
    {        
        const struct cred *cred = current_cred();

        sufs_send_file_to_checker(ino, file_type, index_offset, tgid, 
            cred->uid.val, cred->gid.val);
        ret = sufs_get_result_from_checker(ino);

        /* passed */
        if (ret == SUFS_CHECKER_PASS_RET_CODE)
        {
            sufs_kfs_free_backup_metadata(sinode->file_type, 
                sinode->shadow_index_offset, ino);

            sinode->file_type = file_type;
            sinode->index_offset = index_offset; 
        }
        else
        {
            /* revert to the checkpoint */
            sinode->index_offset = sinode->shadow_index_offset;
            sinode->shadow_index_offset = 0;
        }
    }

    if ((ret == SUFS_CHECKER_PASS_RET_CODE) && 
            (sinode->file_type == SUFS_FILE_TYPE_DIR))
    {
        sufs_kfs_dir_update_sinode(sinode, ino);
    }

    return ret;
}

long sufs_do_unmap_file(struct sufs_super_block *sb, int ino, char file_type, 
                        unsigned long index_offset)
{
    struct sufs_tgroup *tgroup = NULL;

    struct vm_area_struct *vma = NULL;

    struct sufs_shadow_inode *sinode = NULL;

    struct sufs_kfs_lease * lease = NULL;

    long ret = 0;

    int tgid = 0;

    tgid = sufs_kfs_pid_to_tgid(current->tgid, 0);

    tgroup = sufs_kfs_tgid_to_tgroup(tgid);

    if (tgroup == NULL)
    {
        printk("Cannot find the tgroup with pid :%d\n", current->tgid);
        return -ENODEV;
    }

    vma = tgroup->mount_vma;

    if (vma == NULL)
    {
        printk("Cannot find the mapped vma\n");
        return -ENODEV;
    }


    sinode = sufs_find_sinode(ino);

    if (sinode == NULL)
    {
        printk("Cannot find sinode with ino %d\n", ino);
        return -EINVAL;
    }

    if (sinode->file_type == SUFS_FILE_TYPE_NONE)
    {
        printk("Cannot unmap an unexisting sinode with ino %d\n", ino);
        return -ELOOP;    
    }

    lease = &(sinode->lease);

    if (lease == NULL)
    {
        printk("sinode with empty lease %d\n", ino);
        return -EINVAL;
    }

    if ((ret = sufs_kfs_release_lease(ino, lease, tgid)) < 0)
    {
        printk("releasing lease error with ino: %d\n", ino);
        return ret;
    }

    return sufs_do_real_unmap_file(sb, ino, file_type, index_offset, 
        sinode, vma, tgroup, tgid);
}

long sufs_unmap_file(unsigned long arg)
{
    long ret = 0;
    struct sufs_ioctl_map_entry entry;

    if (copy_from_user(&entry, (void*) arg,
            sizeof(struct sufs_ioctl_map_entry)))
        return -EFAULT;

    ret = sufs_do_unmap_file(&sufs_sb, entry.inode, entry.file_type, 
            entry.index_offset);

    return ret;
}

static int sufs_can_chown(void)
{
    const struct cred * cred = NULL;

    cred = current_cred();

    /* Simplified, only root can chown */
    return (cred->uid.val == 0);
}

static int sufs_do_chown(int ino, int owner, int group,
        unsigned long inode_offset)
{
    struct sufs_shadow_inode *sinode = NULL;
    struct sufs_inode * inode = NULL;

    if (!sufs_can_chown())
        return -EPERM;

    sinode = sufs_find_sinode(ino);

    if (sinode == NULL)
    {
        printk("Cannot find sinode with ino %d\n", ino);
        return -EINVAL;
    }

    inode = (struct sufs_inode * ) sufs_kfs_offset_to_virt_addr(inode_offset);

    if (owner > 0)
    {
        sinode->uid = owner;

        if (inode)
            inode->uid = owner;
    }

    if (group > 0)
    {
        sinode->gid = group;

        if (inode)
            inode->gid = owner;
    }

    return 0;
}

long sufs_chown(unsigned long arg)
{
    struct sufs_ioctl_chown_entry entry;

    if (copy_from_user(&entry, (void*) arg,
            sizeof(struct sufs_ioctl_chown_entry)))
        return -EFAULT;

    return sufs_do_chown(entry.inode, entry.owner, entry.group,
            entry.inode_offset);
}

static int sufs_can_chmod(struct sufs_shadow_inode * sinode)
{
    const struct cred * cred = NULL;

    cred = current_cred();

    /*
     * Simplified:
     * only root can chmod or
     * the owner of the file
     */
    return (cred->uid.val == 0 || sinode->uid == cred->uid.val);
}


static int sufs_do_chmod(int ino, int mode, unsigned long inode_offset)
{
    struct sufs_shadow_inode *sinode = NULL;
    struct sufs_inode * inode = NULL;

    sinode = sufs_find_sinode(ino);

    if (sinode == NULL)
    {
        printk("Cannot find sinode with ino %d\n", ino);
        return -EINVAL;
    }

    if (!sufs_can_chmod(sinode))
        return -EPERM;

    inode = (struct sufs_inode * ) sufs_kfs_offset_to_virt_addr(inode_offset);

    sinode->mode = mode;

    if (inode)
        inode->mode = mode;

    return 0;
}


long sufs_chmod(unsigned long arg)
{
    struct sufs_ioctl_chmod_entry entry;

    if (copy_from_user(&entry, (void*) arg,
            sizeof(struct sufs_ioctl_chmod_entry)))
        return -EFAULT;

    return sufs_do_chmod(entry.inode, entry.mode, entry.inode_offset);
}

static long sufs_do_commit(struct sufs_super_block *sb, int ino, char file_type,
                        unsigned long index_offset)
{
    struct sufs_tgroup *tgroup = NULL;

    struct vm_area_struct *vma = NULL;

    struct sufs_shadow_inode *sinode = NULL;
    
    unsigned long perm = 0;
    /* This is quite stupid */
    pgprot_t prop;

    long ret = 0;

    int tgid = 0;
    const struct cred *cred = current_cred();

    if (!sufs_kfs_checker)
        return -ENODEV; 

    tgid = sufs_kfs_pid_to_tgid(current->tgid, 0);

    tgroup = sufs_kfs_tgid_to_tgroup(tgid);

    if (tgroup == NULL)
    {
        printk("Cannot find the tgroup with pid :%d\n", current->tgid);
        return -ENODEV;
    }

    vma = tgroup->mount_vma;

    if (vma == NULL)
    {
        printk("Cannot find the mapped vma\n");
        return -ENODEV;
    }

    sinode = sufs_find_sinode(ino);

    if (sinode == NULL)
    {
        printk("Cannot find sinode with ino %d\n", ino);
        return -EINVAL;
    }

    if (sinode->file_type == SUFS_FILE_TYPE_NONE)
    {
        printk("Cannot unmap an unexisting sinode with ino %d\n", ino);
        return -ELOOP;
    }

    if ((ret = sufs_unmap_file_pages(sb, index_offset, vma, tgid) < 0))
    {
        printk("unmapping file pages error with ino: %d\n", ino);
        return ret;
    }

    if (sufs_kfs_checker)
    {
        sufs_send_file_to_checker(ino, file_type, index_offset, tgid, 
                cred->uid.val, cred->gid.val);
        ret = sufs_get_result_from_checker(ino);
    }
    else
    {
        ret = SUFS_CHECKER_PASS_RET_CODE;
    }

    if (ret != SUFS_CHECKER_PASS_RET_CODE)
    {
        printk("Validation failed with ino %d\n", ino);
        ret = -EINVAL; 
        goto out; 
    }

    /* passed */
    sufs_kfs_free_backup_metadata(sinode->file_type,
                                  sinode->shadow_index_offset, ino);

    sinode->file_type = file_type; 
    sinode->index_offset = index_offset; 

    if ((ret == SUFS_CHECKER_PASS_RET_CODE) && 
            (file_type == SUFS_FILE_TYPE_DIR))
    {
        sufs_kfs_dir_update_sinode(sinode, ino);
    }


    sufs_kfs_backup_file_metadata(sinode);
 
out:
    perm = sufs_gen_access_perm(sinode, 1);
    prop = vm_get_page_prot(perm);

    sufs_map_file_pages(sb, sinode, vma, prop, tgid, ino);
    return ret;
}

long sufs_commit(unsigned long arg)
{
    struct sufs_ioctl_commit_entry entry;

    if (copy_from_user(&entry, (void *)arg,
                       sizeof(struct sufs_ioctl_commit_entry)))
        return -EFAULT;

    return sufs_do_commit(&sufs_sb, entry.inode, entry.file_type, 
            entry.index_offset);
}