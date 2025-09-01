#ifndef SUFS_KFS_RING_H_
#define SUFS_KFS_RING_H_

#include "../include/kfs_config.h"
#include "tgroup.h"

int sufs_kfs_create_ring(struct sufs_tgroup * tgroup);

void sufs_kfs_delete_ring(struct sufs_tgroup * tgroup);

int sufs_kfs_allocate_pages(unsigned long size, int node,
        unsigned long ** kaddr, struct page ** kpage);

int sufs_kfs_vmalloc_pages(unsigned long size, unsigned long **kaddr);

int sufs_kfs_mmap_pages_to_user(unsigned long addr, unsigned long size,
        struct vm_area_struct * vma, int user_writable, struct page * pg);

int sufs_kfs_mmap_vmalloc_pages_to_user(unsigned long kaddr,
        unsigned long uaddr, unsigned long size,
        struct vm_area_struct * vma, int user_writable);
        
int sufs_kfs_mmap_ring(unsigned long addr, unsigned long size,
        struct vm_area_struct * vma, int user_writable,
        unsigned long ** kaddr, struct page ** kpage, int node);

int sufs_kfs_mmap_ring_vmalloc(unsigned long addr, 
        unsigned long size, struct vm_area_struct * vma, 
        int user_writable, unsigned long ** kaddr);

#endif
