#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>


#include "../include/libfs_config.h"
#include "../include/common_inode.h"
#include "../include/ring_buffer.h"
#include "../include/ioctl.h"

#include "mfs.h"
#include "mnode.h"
#include "util.h"
#include "cmd.h"
#include "delegation.h"
#include "journal.h"
#include "tls.h"
#include "stat.h"
#include "simple_ring_buffer.h"
#include "proc.h"

struct sufs_libfs_mnode *sufs_libfs_root_dir = NULL;


atomic_char *sufs_libfs_inode_mapped_attr = NULL;

atomic_char * sufs_libfs_inode_has_mapped = NULL;

atomic_char * sufs_libfs_inode_has_index = NULL;

pthread_spinlock_t * sufs_libfs_inode_map_lock = NULL;

struct sufs_libfs_mapped_inodes
{
    int * inodes;
    pthread_spinlock_t lock;
    int index;
};

struct sufs_libfs_wanted_entry
{
    int ino;
    struct sufs_libfs_wanted_entry * next;
};

struct sufs_libfs_wanted_inodes
{
    struct sufs_libfs_wanted_entry * head;
    pthread_spinlock_t lock;
    volatile int count;
};

static struct sufs_libfs_mapped_inodes sufs_libfs_mapped_inodes;
static struct sufs_libfs_wanted_inodes sufs_libfs_wanted_inodes;

void sufs_libfs_mfs_init(void)
{
    int i = 0;
    sufs_libfs_inode_mapped_attr = calloc(1, SUFS_MAX_INODE_NUM / sizeof(char));

    sufs_libfs_inode_has_mapped = calloc(1, SUFS_MAX_INODE_NUM / sizeof(char));

    sufs_libfs_inode_has_index = calloc(1, SUFS_MAX_INODE_NUM / sizeof(char));

    sufs_libfs_mapped_inodes.inodes = calloc(SUFS_MAX_MAP_FILE, sizeof(int));

    sufs_libfs_wanted_inodes.head = NULL; 

    sufs_libfs_inode_map_lock = calloc(SUFS_LIBFS_FILE_MAP_LOCK_SIZE,
            sizeof(pthread_spinlock_t));

    if (sufs_libfs_inode_mapped_attr    == NULL    ||
        sufs_libfs_inode_has_mapped     == NULL    ||
        sufs_libfs_inode_has_index      == NULL    ||
        sufs_libfs_mapped_inodes.inodes == NULL    ||
        sufs_libfs_inode_map_lock       == NULL)
    {
        fprintf(stderr, "Cannot allocate sufs_inode_mapped!\n");
        abort();
    }

    sufs_libfs_mapped_inodes.index = 0;
    pthread_spin_init(&(sufs_libfs_mapped_inodes.lock), PTHREAD_PROCESS_SHARED);

    sufs_libfs_wanted_inodes.count = 0;
    pthread_spin_init(&(sufs_libfs_wanted_inodes.lock), PTHREAD_PROCESS_SHARED);

    for (i = 0; i < SUFS_LIBFS_FILE_MAP_LOCK_SIZE; i++)
    {
        pthread_spin_init(&(sufs_libfs_inode_map_lock[i]),
                PTHREAD_PROCESS_SHARED);
    }
}

void sufs_libfs_mfs_fini(void)
{
    if (sufs_libfs_inode_mapped_attr)
        free(sufs_libfs_inode_mapped_attr);

    if (sufs_libfs_inode_has_mapped)
        free(sufs_libfs_inode_has_mapped);

    if (sufs_libfs_inode_has_index)
        free(sufs_libfs_inode_has_index);

    if (sufs_libfs_mapped_inodes.inodes)
        free(sufs_libfs_mapped_inodes.inodes);

    if (sufs_libfs_inode_map_lock)
        free((void *) sufs_libfs_inode_map_lock);
}


void sufs_libfs_mfs_add_mapped_inode(int ino)
{
    int index = 0;
    if (sufs_libfs_bm_test_bit(sufs_libfs_inode_has_mapped, ino))
        return;

    pthread_spin_lock(&(sufs_libfs_mapped_inodes.lock));

    if (sufs_libfs_bm_test_bit(sufs_libfs_inode_has_mapped, ino))
        goto out;
    else
    {
        sufs_libfs_bm_set_bit(sufs_libfs_inode_has_mapped, ino);
    }

    if ((index = sufs_libfs_mapped_inodes.index) >= SUFS_MAX_MAP_FILE)
    {
        fprintf(stderr, "index exceed SUFS_MAX_MAP_FILE!\n");
        goto out;
    }

    sufs_libfs_mapped_inodes.inodes[index] = ino;
    sufs_libfs_mapped_inodes.index++;

 out:
    pthread_spin_unlock(&(sufs_libfs_mapped_inodes.lock));
}

void sufs_libfs_mfs_unmap_mapped_inodes(void)
{
    int i = 0;

    pthread_spin_lock(&(sufs_libfs_mapped_inodes.lock));

    for (i = 0; i < sufs_libfs_mapped_inodes.index; i++)
    {
        int ino = sufs_libfs_mapped_inodes.inodes[i];

        if (sufs_libfs_bm_test_bit((atomic_char*) SUFS_MAPPED_RING_ADDR, ino))
        {
            struct sufs_libfs_mnode *mnode = sufs_libfs_mnode_array[ino];
            unsigned long index_offset = 0;

            if (mnode == NULL) 
                continue;

            index_offset = 
                sufs_libfs_virt_addr_to_offset((unsigned long)mnode->index_start);

            sufs_libfs_cmd_unmap_file(ino, mnode->type, index_offset);
        }
    }

    pthread_spin_unlock(&(sufs_libfs_mapped_inodes.lock));
}

void sufs_libfs_fs_init(void)
{
    struct sufs_inode * inode = calloc(1, sizeof(struct sufs_inode));
    inode->file_type = SUFS_FILE_TYPE_DIR;
    inode->mode = SUFS_ROOT_PERM;

    inode->uid = inode->gid = 0;
    inode->size = 0;
    inode->offset = 0;

    sufs_libfs_root_dir = sufs_libfs_mfs_mnode_init(SUFS_FILE_TYPE_DIR,
            SUFS_ROOT_INODE, SUFS_ROOT_INODE, inode);

    sufs_libfs_cmd_mount();
}

void sufs_libfs_fs_fini(void)
{
    if (sufs_libfs_root_dir->inode)
        free(sufs_libfs_root_dir->inode);

    sufs_libfs_mfs_unmap_mapped_inodes();

    sufs_libfs_cmd_umount();
}

/*
 * Copy the next path element from path into name.
 * Update the pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path=='\0' to see if the name is the last one.
 *
 * If copied into name, return 1.
 * If no name to remove, return 0.
 * If the name is longer than DIRSIZ, return -1;
 *
 * Examples:
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */

static int skipelem(char **rpath, char *name)
{
    char *path = *rpath;
    char *s = NULL;
    int len = 0;

    while (*path == '/')
        path++;

    if (*path == 0)
        return 0;

    s = path;

    while (*path != '/' && *path != 0)
        path++;

    len = path - s;

    if (len > SUFS_NAME_MAX)
    {
        fprintf(stderr, "Error: Path component longer than SUFS_NAME_SIZE"
                " (%d characters)\n", SUFS_NAME_MAX);
        return -1;
    }
    else
    {
        memmove(name, s, len);
        if (len < SUFS_NAME_MAX)
        {
            name[len] = 0;
        }
    }

    while (*path == '/')
        path++;

    *rpath = path;
    return 1;
}

/*
 * Look up and return the mnode for a path name.  If nameiparent is true,
 * return the mnode for the parent and copy the final path element into name.
 */

struct sufs_libfs_mnode* sufs_libfs_namex(struct sufs_libfs_mnode *cwd,
        char *path, bool nameiparent, char *name, int map)
{
    struct sufs_libfs_mnode *m;
    int r;

    if (*path == '/')
    {
        m = sufs_libfs_root_dir;
    }
    else
    {
        m = cwd;
    }

    while ((r = skipelem(&path, name)) == 1)
    {
        struct sufs_libfs_mnode *next = NULL;

        if (sufs_libfs_mnode_type(m) != SUFS_FILE_TYPE_DIR)
            return NULL;

        if (nameiparent && *path == '\0')
        {
            /* Stop one level early */
            return m;
        }

        next = sufs_libfs_mnode_dir_lookup(m, name, map);

        if (!next)
            return NULL;

        m = next;
    }

    if (r == -1 || nameiparent)
        return NULL;



    return m;
}


s64 sufs_libfs_readm(struct sufs_libfs_mnode *m, char *buf, u64 start,
        u64 nbytes)
{
    u64 end = sufs_libfs_mnode_file_size(m);
    u64 off = 0;
    int delegated = 0, cpt_idx = 0, index = 0;

    long issued_cnt[SUFS_PM_MAX_INS];
    struct sufs_notifyer * completed_cnt = NULL;

#if SUFS_LIBFS_RANGE_LOCK
    unsigned long start_seg = 0, end_seg = 0;
#endif

    if (sufs_libfs_map_file(m, 0) != 0)
    {
        return -1; 
    }


    if (start + nbytes < end)
    {
        end = start + nbytes;
    }

    if (sufs_libfs_delegation)
    {
        index = sufs_libfs_tls_my_index();

        memset(issued_cnt, 0, sizeof(long) * SUFS_PM_MAX_INS);

        completed_cnt = sufs_libfs_tls_data[index].cpt_cnt;

        if (completed_cnt == NULL)
        {
            completed_cnt = sufs_libfs_get_completed_cnt(index);
        }

        memset(completed_cnt, 0,
               sizeof(struct sufs_notifyer) * SUFS_PM_MAX_INS);

        cpt_idx = sufs_libfs_tls_data[index].cpt_idx;
    }

    sufs_libfs_inode_read_lock(m);

#if SUFS_LIBFS_RANGE_LOCK
    start_seg = start >> SUFS_LIBFS_SEGMENT_SIZE_BITS;
    end_seg = end >> SUFS_LIBFS_SEGMENT_SIZE_BITS;
    sufs_libfs_irange_lock_read_lock(&m->data.file_data.range_lock, start_seg,
            end_seg);
#endif


    while (start + off < end)
    {
        u64 pos = start + off, pgbase = FILE_BLOCK_ROUND_DOWN(pos), pgoff = 0,
                pgend = 0, len = 0;

        unsigned long addr = sufs_libfs_mnode_file_get_page(m,
                pgbase / SUFS_FILE_BLOCK_SIZE);

        if (!addr || !sufs_libfs_file_is_mapped(m))
            break;

        pgoff = pos - pgbase;
        pgend = end - pgbase;

        if (pgend > SUFS_FILE_BLOCK_SIZE)
            pgend = SUFS_FILE_BLOCK_SIZE;

        len = pgend - pgoff;

        if (!sufs_libfs_delegation || len < SUFS_READ_DELEGATION_LIMIT)
        {
            memcpy(buf + off, (char*) addr + pgoff, len);
        }
        else
        {
            delegated = 1;
            sufs_libfs_do_read_delegation(&sufs_libfs_sb,
                (unsigned long) (buf + off),
                sufs_libfs_virt_addr_to_offset((unsigned long) (addr + pgoff)),
                len, 0, issued_cnt, cpt_idx, 1);
        }

        off += (pgend - pgoff);
    }

    if (delegated)
    {
        sufs_libfs_complete_delegation(&sufs_libfs_sb, issued_cnt, completed_cnt);
    }

#if SUFS_LIBFS_RANGE_LOCK
    sufs_libfs_irange_lock_read_unlock(&m->data.file_data.range_lock, start_seg,
            end_seg);
#endif

    sufs_libfs_inode_read_unlock(m);

    return off;
}

s64 sufs_libfs_writem(struct sufs_libfs_mnode *m, char *buf, u64 start,
        u64 nbytes)
{
    u64 end = start + nbytes;
    u64 off = 0;
    u64 size = sufs_libfs_mnode_file_size(m);
    int whole_lock = 0, cpt_idx = 0;
#if SUFS_LIBFS_RANGE_LOCK
        unsigned long start_seg = 0, end_seg = 0;
#endif

    int delegated = 0, index = 0;

    long issued_cnt[SUFS_PM_MAX_INS];
    struct sufs_notifyer * completed_cnt = NULL;

    SUFS_LIBFS_DEFINE_TIMING_VAR(writem_time);
    SUFS_LIBFS_DEFINE_TIMING_VAR(index_time);

    if (sufs_libfs_map_file(m, 1) != 0)
    {
        return -1; 
    }

    SUFS_LIBFS_START_TIMING(SUFS_LIBFS_WRITEM, writem_time);

    if (sufs_libfs_delegation)
    {
        index = sufs_libfs_tls_my_index();

        memset(issued_cnt, 0, sizeof(long) * SUFS_PM_MAX_INS);

        completed_cnt = sufs_libfs_tls_data[index].cpt_cnt;

        if (completed_cnt == NULL)
        {
            completed_cnt = sufs_libfs_get_completed_cnt(index);
        }

        memset(completed_cnt, 0,
               sizeof(struct sufs_notifyer) * SUFS_PM_MAX_INS);

        cpt_idx = sufs_libfs_tls_data[index].cpt_idx;
    }


    /*
     * Here we simplified the code to not consider sparse files. Handling
     * spare file is simple, just iterate to find whether there is an
     * page not mapped
     */

    if (end > size)
        whole_lock = 1;

    if (whole_lock)
    {
        sufs_libfs_inode_write_lock(m);
    }
    else
    {
        sufs_libfs_inode_read_lock(m);

#if SUFS_LIBFS_RANGE_LOCK
        start_seg = start >> SUFS_LIBFS_SEGMENT_SIZE_BITS;
        end_seg = end >> SUFS_LIBFS_SEGMENT_SIZE_BITS;
        sufs_libfs_irange_lock_write_lock(&m->data.file_data.range_lock,
                start_seg, end_seg);
#endif
    }


    while (start + off < end)
    {
        u64 pos = start + off, pgbase = FILE_BLOCK_ROUND_DOWN(pos),
                pgoff = pos - pgbase, pgend = end - pgbase, len = 0;

        unsigned long addr = 0;
        unsigned long block = 0;

        int need_resize = 0;

        if (pgend > SUFS_FILE_BLOCK_SIZE)
            pgend = SUFS_FILE_BLOCK_SIZE;

        len = pgend - pgoff;

        SUFS_LIBFS_START_TIMING(SUFS_LIBFS_INDEX, index_time);
        addr = sufs_libfs_mnode_file_get_page(m, pgbase / SUFS_FILE_BLOCK_SIZE);
        SUFS_LIBFS_END_TIMING(SUFS_LIBFS_INDEX, index_time);

        if (addr)
        {
            if (pos + len > sufs_libfs_mnode_file_size(m))
            {
                need_resize = 1;
            }

            /*
             * What happens when writing past the end of the file but within
             * the file's last page?  One worry might be that we're exposing
             * some non-zero bytes left over in the part of the last page that
             * is past the end of the file.  Our plan is to ensure that any
             * file truncate zeroes out any partial pages.  Currently, we only
             * have O_TRUNC, which discards all pages.
             */

            if (!sufs_libfs_delegation || len < SUFS_WRITE_DELEGATION_LIMIT)
            {
                memcpy((char*) addr + pgoff, buf + off, len);
                sufs_libfs_clwb_buffer((char *) addr + pgoff, len, 0);
            }
            else
            {
                delegated = 1;
                sufs_libfs_do_write_delegation(&sufs_libfs_sb,
                    (unsigned long) (buf + off),
                    sufs_libfs_virt_addr_to_offset((unsigned long) (addr + pgoff)),
                    len, 0, 1, 0, issued_cnt, cpt_idx, 1, index);
            }

            if (need_resize)
                sufs_libfs_mnode_file_resize_nogrow(m, pos + len);
        }
        else
        {
            u64 msize = 0;

            unsigned long addr = 0;

            msize = sufs_libfs_mnode_file_size(m);
            /*
             * If this is a write past the end of the file, we may need
             * to first zero out some memory locations, or even fill in
             * a few zero pages.  We do not support sparse files -- the
             * holes are filled in with zeroed pages.
             */
            while (msize < pgbase)
            {
                if (msize % SUFS_FILE_BLOCK_SIZE)
                {
                    sufs_libfs_mnode_file_resize_nogrow(m,
                            msize - (msize % SUFS_FILE_BLOCK_SIZE) + SUFS_FILE_BLOCK_SIZE);

                    msize = msize - (msize % SUFS_FILE_BLOCK_SIZE) + SUFS_FILE_BLOCK_SIZE;
                }
                else
                {
                    SUFS_LIBFS_START_TIMING(SUFS_LIBFS_INDEX, index_time);

                    sufs_libfs_new_file_data_blocks(&sufs_libfs_sb, m, &block,
                            SUFS_FILE_BLOCK_PAGE_CNT, 1);

                    SUFS_LIBFS_END_TIMING(SUFS_LIBFS_INDEX, index_time);

                    if (!block)
                        break;

                    addr = sufs_libfs_block_to_virt_addr((unsigned long) block);

                    sufs_libfs_mnode_file_resize_append(m,
                            msize + SUFS_FILE_BLOCK_SIZE, addr);

                    msize += SUFS_FILE_BLOCK_SIZE;
                }
            }

            sufs_libfs_new_file_data_blocks(&sufs_libfs_sb, m, &block,
                    SUFS_FILE_BLOCK_PAGE_CNT, 0);

            if (!block)
                break;

            addr = sufs_libfs_block_to_virt_addr((unsigned long) block);

            if (!sufs_libfs_delegation || len < SUFS_WRITE_DELEGATION_LIMIT)
            {
                memcpy((void *) addr + pgoff, buf + off, len);
                sufs_libfs_clwb_buffer((char *) addr + pgoff, len, 0);
            }
            else
            {

                delegated = 1;

                sufs_libfs_do_write_delegation(&sufs_libfs_sb,
                    (unsigned long) (buf + off),
                    sufs_libfs_virt_addr_to_offset((unsigned long) (addr + pgoff)),
                    len, 0, 1, 0, issued_cnt, cpt_idx, 1, index);
            }

            SUFS_LIBFS_START_TIMING(SUFS_LIBFS_INDEX, index_time);

            sufs_libfs_mnode_file_resize_append(m, pos + len, addr);

            SUFS_LIBFS_END_TIMING(SUFS_LIBFS_INDEX, index_time);
        }

        off += len;

        if (!sufs_libfs_file_is_mapped(m))
            break;
    }

    if (delegated)
    {
        sufs_libfs_complete_delegation(&sufs_libfs_sb, issued_cnt, completed_cnt);
    }

    if (whole_lock)
    {
#if SUFS_LIBFS_RANGE_LOCK
        sufs_libfs_irange_lock_resize(&m->data.file_data.range_lock,
                start + off);
#endif
        sufs_libfs_inode_write_unlock(m);
    }
    else
    {
#if SUFS_LIBFS_RANGE_LOCK
        sufs_libfs_irange_lock_write_unlock(&m->data.file_data.range_lock,
                start_seg, end_seg);
#endif

        sufs_libfs_inode_read_unlock(m);
    }

    sufs_libfs_sfence();

    SUFS_LIBFS_END_TIMING(SUFS_LIBFS_WRITEM, writem_time);

    return off;
}

/* Flush all the index from start to end */
void sufs_libfs_flush_file_index(struct sufs_fidx_entry * start,
        struct sufs_fidx_entry * end)
{
    struct sufs_fidx_entry * idx = NULL;

    if (start == end)
    {
        sufs_libfs_clwb_buffer(&(start->offset), sizeof(start->offset), 0);
        return;
    }

    start = (struct sufs_fidx_entry *)
            CACHE_ROUND_DOWN((unsigned long) start);

    end = (struct sufs_fidx_entry *)
            CACHE_ROUND_DOWN((unsigned long) start);

    idx = start;

    while (idx->offset != 0)
    {
        if ( ((unsigned long) idx % SUFS_CACHELINE) == 0)
            sufs_libfs_clwb_buffer(idx, SUFS_CACHELINE, 0);

        if (likely(sufs_is_norm_fidex(idx)))
        {
            idx++;
        }
        else
        {
            idx = (struct sufs_fidx_entry*)
                    sufs_libfs_offset_to_virt_addr(idx->offset);
        }

        if (idx == end)
            break;
    }
}

int sufs_libfs_truncatem(struct sufs_libfs_mnode *m, off_t length)
{
    unsigned long msize = sufs_libfs_mnode_file_size(m);
    unsigned long mbase = FILE_BLOCK_ROUND_UP(msize),
            lbase = FILE_BLOCK_ROUND_UP(length);

    unsigned long block = 0;

    struct sufs_fidx_entry * idx = NULL;

    int i = 0;

    if (sufs_libfs_map_file(m, 1) != 0)
    {
        return -1;
    }


    sufs_libfs_inode_write_lock(m);

    if (!sufs_libfs_file_is_mapped(m))
    {
        sufs_libfs_inode_write_unlock(m);
        return -1;
    }

    if (mbase == lbase)
    {
        m->data.file_data.size_ = length;
    }
    else if (mbase < lbase)
    {
        struct sufs_fidx_entry * start_idx = NULL, * end_idx = NULL;

        unsigned long bcount = (lbase - mbase) / SUFS_PAGE_SIZE;

        sufs_libfs_new_file_data_blocks(&sufs_libfs_sb, m, &block,
                bcount, 0);

        if (!block)
            return -1;

        mbase = mbase / SUFS_FILE_BLOCK_SIZE;

        for (i = 0 ; i < bcount; i+= SUFS_FILE_BLOCK_PAGE_CNT)
        {
            unsigned long addr = sufs_libfs_block_to_virt_addr(block + i);

            idx = sufs_libfs_mnode_file_index_append(m, addr);

            sufs_libfs_mnode_file_fill_index(m,
                    mbase + (i / SUFS_FILE_BLOCK_PAGE_CNT), (unsigned long) idx);

            if (i == 0)
                start_idx = idx;
        }

        end_idx = idx;

        sufs_libfs_flush_file_index(start_idx, end_idx);
        sufs_libfs_sfence();

        m->data.file_data.size_ = length;
#if SUFS_LIBFS_RANGE_LOCK
        sufs_libfs_irange_lock_resize(&m->data.file_data.range_lock, length);
#endif
    }
    else
    {
        unsigned long old_idx = 0;

        /* Do not delete the page that holds idx */
        int first = 1, persist = 0;

        if (lbase == 0)
        {
            sufs_libfs_mnode_file_truncate_zero(m);
            return 0;
        }

        idx = sufs_libfs_mnode_file_get_idx(m, lbase / SUFS_FILE_BLOCK_SIZE);

        while (idx->offset != 0)
        {
            if (likely(sufs_is_norm_fidex(idx)))
            {
                block = sufs_libfs_offset_to_block(idx->offset);

                /*
                 * Here seems no need for further validation
                 * if the head of the block is owned, then the rest of the
                 * block must be owned
                 */
                if (sufs_libfs_is_block_owned(block))
                {
                    sufs_libfs_free_data_blocks(&sufs_libfs_sb, block,
                            SUFS_FILE_BLOCK_PAGE_CNT);
                }

                idx->offset = 0;

                /* persist the first write to idx->offset */
                if (!persist)
                {
                    sufs_libfs_clwb_buffer(&idx->offset, 
                        sizeof(idx->offset), 0);
                    sufs_libfs_sfence();
                    persist = 1;
                }

                idx++;
            }
            else
            {
                unsigned long nidx = 0;

                old_idx = (unsigned long) idx;

                nidx = (unsigned long) sufs_libfs_offset_to_virt_addr(idx->offset);

                idx->offset = 0;

                /* persist the first write to idx->offset */
                if (!persist)
                {
                    sufs_libfs_clwb_buffer(&idx->offset, 
                            sizeof(idx->offset), 0);
                    sufs_libfs_sfence();
                    persist = 1;
                }


                idx = (struct sufs_fidx_entry*) nidx;

                if (!first)
                {
                    block = sufs_libfs_virt_addr_to_block(old_idx);

                    if (sufs_libfs_is_block_owned(block))
                    {
                        sufs_libfs_free_index_block(&sufs_libfs_sb, block);
                    }
                }

                first = 0;
            }
        }

        old_idx = (unsigned long) idx;

        if (!first)
        {
            block = sufs_libfs_virt_addr_to_block(old_idx);

            /*
             * Here seems no need for further validation
             * if the head of the block is owned, then the rest of the
             * block must be owned
             */
            if (sufs_libfs_is_block_owned(block))
            {
                sufs_libfs_free_index_block(&sufs_libfs_sb, block);
            }
        }

        /*
         * Since we have updated the data size, we shouldn't need to update the
         * indexing structure
         */

        m->index_end = idx;
        m->data.file_data.size_ = length;
    }

    sufs_libfs_inode_write_unlock(m);

    return 0;

}

int sufs_libfs_do_map_file(struct sufs_libfs_mnode *m, int writable)
{
    int ret = 0;

    unsigned long index_offset = 0;

    while ((ret = sufs_libfs_cmd_map_file(m->ino_num, writable, &index_offset))
            != 0)
    {
        if (ret == -EAGAIN)
        {
            usleep(10 * 1000); 
            continue;
        }
        else
        {
            fprintf(stderr, "map ion: %d writable: %d failed with %d\n",
                    m->ino_num, writable, ret);

            return ret;
        }
    }

    if (index_offset == 0)
    {
        m->index_start = NULL;
    }
    else
    {
        m->index_start = (struct sufs_fidx_entry *)
                    sufs_libfs_offset_to_virt_addr(index_offset);
    }

    if (writable)
        sufs_libfs_file_set_writable(m);

    sufs_libfs_mfs_add_mapped_inode(m->ino_num);

    return 0;
}



/* Helper to unmap a file, used in sufs_libfs_map_file */
static int
sufs_libfs_unmap_file_helper(struct sufs_libfs_mnode *m, int writable)
{
    int ino = m->ino_num, ret = 0;
    unsigned long index_offset = 0;

    if (sufs_libfs_mnode_type(m) == SUFS_FILE_TYPE_REG)
    {
        sufs_libfs_inode_write_lock(m);
    }
    else
    {
        sufs_libfs_chainhash_obtain_all_locks(&m->data.dir_data.map_);
    }

    index_offset =
        sufs_libfs_virt_addr_to_offset((unsigned long) m->index_start);

    ret = sufs_libfs_cmd_unmap_file(ino, m->type, index_offset);

    if (sufs_libfs_mnode_type(m) == SUFS_FILE_TYPE_REG)
    {
        sufs_libfs_inode_write_unlock(m);
    }
    else
    {
        sufs_libfs_chainhash_release_all_locks(&m->data.dir_data.map_);
    }

    return ret;
}

static void sufs_libfs_release_wanted_inodes(void)
{
    struct sufs_libfs_wanted_entry *prev = NULL, *iter = NULL;

    if (sufs_libfs_wanted_inodes.count > 0)
    {
        pthread_spin_lock(&(sufs_libfs_wanted_inodes.lock));
        iter = sufs_libfs_wanted_inodes.head;
        while (iter != NULL)
        {
            struct sufs_libfs_mnode *m = sufs_libfs_mnode_array[iter->ino];
            if (sufs_libfs_file_is_mapped(m) && 
                sufs_libfs_file_should_release(m))
            {
                sufs_libfs_unmap_file_helper(m, 1);
                if (prev == NULL)
                {
                    sufs_libfs_wanted_inodes.head = iter->next;
                }
                else
                {
                    prev->next = iter->next;
                }
                 sufs_libfs_wanted_inodes.count--;
            }

            prev = iter;
            iter = iter->next;
        }
        pthread_spin_unlock(&(sufs_libfs_wanted_inodes.lock));

    }
}


static void sufs_libfs_handle_lease(void)
{
    struct sufs_ioctl_to_free_entry e; 
    struct sufs_libfs_wanted_entry * entry = NULL; 

    while (!sufs_libfs_sr_is_empty(sufs_libfs_lease_ring))
    {
        int ret = sufs_libfs_do_sr_receive_request(sufs_libfs_lease_ring, 
                &e);

        if (ret == -SUFS_RBUFFER_AGAIN)
            break;

        pthread_spin_lock(&(sufs_libfs_wanted_inodes.lock));

        entry = calloc(1, sizeof(struct sufs_libfs_wanted_entry));
        entry->ino = e.ino_num;
        entry->next = sufs_libfs_wanted_inodes.head;
        sufs_libfs_wanted_inodes.head = entry;

        sufs_libfs_wanted_inodes.count++;

        pthread_spin_unlock(&(sufs_libfs_wanted_inodes.lock));
    }


    sufs_libfs_release_wanted_inodes();
}


int sufs_libfs_map_file(struct sufs_libfs_mnode *m, int writable)
{
    int ret = 0;

    /* Only supports write-shared now */
    writable = 1;

    sufs_libfs_handle_lease();
    
    if (sufs_libfs_file_is_mapped(m))
        return 0;

 
    sufs_libfs_lock_file_mapping(m);

    if (sufs_libfs_file_is_mapped(m))
        goto out;

    if ((ret = sufs_libfs_do_map_file(m, writable)) != 0)
        goto out;

    if (m->type == SUFS_FILE_TYPE_REG)
    {
        sufs_libfs_file_build_index(m);
    }
    else
    {
        sufs_libfs_mnode_dir_build_index(m);
    }
    
    sufs_libfs_bm_set_bit(sufs_libfs_inode_has_index, m->ino_num);

out:
    sufs_libfs_unlock_file_mapping(m);
    return ret;
}

void sufs_libfs_file_build_index(struct sufs_libfs_mnode *m)
{
    struct sufs_fidx_entry *idx = m->index_start;

    int idx_num = 0;

    if (idx == NULL)
        goto out;

    while (idx->offset != 0)
    {

        if (likely(sufs_is_norm_fidex(idx)))
        {
            sufs_libfs_mnode_file_fill_index(m, idx_num, (unsigned long) idx);

            idx_num++;
            idx++;
            m->data.file_data.size_ += SUFS_FILE_BLOCK_SIZE;
        }
        else
        {
            idx = (struct sufs_fidx_entry*) sufs_libfs_offset_to_virt_addr(
                    idx->offset);
        }
    }

out:

    m->index_end = idx;
}

int sufs_libfs_sys_unmap_by_path(struct sufs_libfs_proc *proc, char * path)
{
    struct sufs_libfs_mnode* m = NULL;
    unsigned long index_offset = 0;

    m = sufs_libfs_namei(proc->cwd_m, path, 0);

    if (!m)
    {
        fprintf(stderr, "Cannot find file: %s\n", path);
        return 0;
    }

    return sufs_libfs_unmap_file_helper(m, 1);
}

