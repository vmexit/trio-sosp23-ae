#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../include/checker_config.h"
#include "../include/common_inode.h"
#include "../include/ioctl.h"
#include "../include/priv_inode.h"

#include "checker.h"
#include "chainhash.h"
#include "simple_ring_buffer.h"

#if 1
#define CHECKER_DEBUG_PRINT(fmt, ...) \
    do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#else
#define CHECKER_DEBUG_PRINT(fmt, ...)
#endif

unsigned long max_block = 0;
int *sufs_inode_state, *sufs_mapped_state, *sufs_page_state;

struct sufs_ring_buffer * sufs_checker_file_ring = NULL; 
struct sufs_ring_buffer * sufs_checker_res_ring = NULL;

struct sufs_checker_chainhash h_page, h_inode, h_name;

static inline 
unsigned long sufs_checker_offset_to_virt_addr(unsigned long offset)
{
    return (SUFS_MOUNT_ADDR + offset);
}

static inline unsigned long sufs_checker_virt_addr_to_offset(unsigned long virt)
{
    return (virt - SUFS_MOUNT_ADDR);
}

static inline unsigned long
sufs_checker_offset_to_block(unsigned long offset)
{
    return (offset >> SUFS_PAGE_SHIFT);
}

static inline unsigned long
sufs_checker_block_to_offset(unsigned long block)
{
    return (block << SUFS_PAGE_SHIFT);
}


static inline int is_inode_range_valid(int ino_num)
{
    return (ino_num >= 2 && ino_num <= SUFS_MAX_INODE_NUM);
}

static inline int is_inode_num_valid(int ino_num, unsigned int tgid)
{
    if (is_inode_range_valid(ino_num))
    {
        return (sufs_inode_state[ino_num] == tgid);
    }
    else
    {
        return false;
    }
}

static inline int is_time_stamp_valid(long time)
{
    return (time >= 0 && time <= MAX_TIME_STAMP);
}


static inline bool is_name_invalid(char *name)
{
    /* name contains '/' */
    if (strchr(name, '/') != NULL)
        return true;

    /* name is "." or ".." */
    if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))
        return true;

    /* name is NULL */
    if (name[0] == 0)
        return true;

    return false;
}


static inline struct sufs_shadow_inode *get_shadow_inode(unsigned long ino)
{
    struct sufs_shadow_inode *shadow_inodes_start =
        (struct sufs_shadow_inode *)(SUFS_MOUNT_ADDR + SUFS_SUPER_PAGE_SIZE);

    return &(shadow_inodes_start[ino]);
}

static inline int is_dup_page(unsigned long offset)
{
    unsigned long block = sufs_checker_offset_to_block(offset);
    
    return !(sufs_checker_chainhash_check_and_insert(&h_page, NULL, 0, block)); 
}

static inline int is_dup_inode(int ino_num)
{
    return !(sufs_checker_chainhash_check_and_insert(&h_inode, NULL, 
                0,  ino_num));
}

static inline int is_dup_name(char * name, int name_size)
{
    return !(sufs_checker_chainhash_check_and_insert(&h_name, name, 
        name_size, 0));
}


static inline bool check_regular_files(unsigned long index_offset, 
    unsigned int tgid)
{
    struct sufs_fidx_entry *idx = NULL;

    if (index_offset == 0)
        return true;

    if (is_dup_page(index_offset)) 
    {
        CHECKER_DEBUG_PRINT("check_regular_files: index_offset %lx is duplicate page\n", index_offset);
        return false; 
    }

    idx = (struct sufs_fidx_entry *)
            sufs_checker_offset_to_virt_addr(index_offset);

    while (idx->offset != 0)
    {
        if (sufs_is_norm_fidex(idx))
        {
            if (is_dup_page(idx->offset)) 
            {
                CHECKER_DEBUG_PRINT("check_regular_files: data page offset %lx is duplicate\n", idx->offset);
                return false;
            }

            idx++;
        }
        else
        {
            if (is_dup_page(idx->offset)) 
            {
                CHECKER_DEBUG_PRINT("check_regular_files: index page offset %lx is duplicate\n", idx->offset);
                return false;
            }

            idx = (struct sufs_fidx_entry*)
                    sufs_checker_offset_to_virt_addr(idx->offset);
        }
    }

    return true;
}

static bool is_inode_valid(struct sufs_inode *i, int inode_num, int uid, 
        int gid)
{
    /* file_type must be either directory or regular file */
    if (i->file_type < SUFS_FILE_TYPE_REG || i->file_type > SUFS_FILE_TYPE_DIR) {
        CHECKER_DEBUG_PRINT("is_inode_valid: file_type invalid (file_type=%d)\n", i->file_type);
        return false;
    }

    if (i->mode > 0777)
    {
        CHECKER_DEBUG_PRINT("is_inode_valid: i->mode invalid: %d\n", i->mode);
        return false;
    }

    if (i->uid != uid || i->gid != gid) {
        CHECKER_DEBUG_PRINT("is_inode_valid: uid/gid mismatch (uid=%d/%d, gid=%d/%d)\n", i->uid, uid, i->gid, gid);
        return false;
    }

    if ((!is_time_stamp_valid(i->atime)) || (!is_time_stamp_valid(i->ctime)) ||
        (!is_time_stamp_valid(i->mtime))) {
        CHECKER_DEBUG_PRINT("is_inode_valid: timestamp invalid (atime=%ld, ctime=%ld, mtime=%ld)\n", i->atime, i->ctime, i->mtime);
        return false; 
    }

    return true;
}


static bool is_dentry_valid(struct sufs_dir_entry *d, int tgid, int uid, 
    int gid)
{
    if (d->name_len == 0 || d->name_len != (strlen(d->name) + 1)) {
        CHECKER_DEBUG_PRINT("is_dentry_valid: name_len invalid (name_len=%d, strlen(name)=%lu, name=%s)\n", d->name_len, (unsigned long)strlen(d->name), d->name);
        return false;
    }

    if (d->rec_len != sizeof(struct sufs_dir_entry) + d->name_len) {
        CHECKER_DEBUG_PRINT("is_dentry_valid: rec_len invalid (rec_len=%d, expected=%lu)\n", d->rec_len, (unsigned long)(sizeof(struct sufs_dir_entry) + d->name_len));
        return false;
    }

    if (is_name_invalid(d->name)) {
        CHECKER_DEBUG_PRINT("is_dentry_valid: name invalid (name=%s)\n", d->name);
        return false;
    }

    if (!is_inode_num_valid(d->ino_num, tgid)) 
    {
        CHECKER_DEBUG_PRINT("is_dentry_valid: ino_num invalid (ino_num=%d, tgid=%d)\n", d->ino_num, tgid);
        return false;
    }

    if (is_dup_inode(d->ino_num)) {
        CHECKER_DEBUG_PRINT("is_dentry_valid: duplicate inode (ino_num=%d)\n", d->ino_num);
        return false; 
    }

    if (!(is_inode_valid(&d->inode, d->ino_num, uid, gid))) {
        CHECKER_DEBUG_PRINT("is_dentry_valid: inode invalid (ino_num=%d)\n", d->ino_num);
        return false;
    }

    return true;
}

static inline bool is_dir_empty(struct sufs_shadow_inode *s)
{
    struct sufs_fidx_entry *idx = (struct sufs_fidx_entry *)
            sufs_checker_offset_to_virt_addr(s->shadow_index_offset);

    while (idx->offset != 0)
    {
        if (sufs_is_norm_fidex(idx))
        {
            struct sufs_dir_entry *d = NULL;

            d = (struct sufs_dir_entry *)
                sufs_checker_offset_to_virt_addr(idx->offset);

            while (d->name_len != 0)
            {
                if (d->ino_num != SUFS_INODE_TOMBSTONE)
                {
                    return false;
                }

                d = (struct sufs_dir_entry *)((unsigned long)d + d->rec_len);
            }
            idx++;
        }
        else
        {
            idx = (struct sufs_fidx_entry *)
                sufs_checker_offset_to_virt_addr(idx->offset);
        }
    }

    return true; 
}


static inline bool can_inode_deleted(int ino_num, int parent, int tgid)
{
    struct sufs_shadow_inode *s = NULL; 
    s = get_shadow_inode(ino_num);

    if (s->file_type == SUFS_FILE_TYPE_REG)
            return true;
    else
    {
        if (s->parent != parent)
        {
            return true; 
        }

        if (sufs_mapped_state[ino_num] != 0 && 
            sufs_mapped_state[ino_num] != tgid)
        {
            return false; 
        }

        return is_dir_empty(s);
    }
}


static inline 
bool is_disconnected(int ino_num, int tgid)
{
    struct sufs_shadow_inode *s = get_shadow_inode(ino_num);
    struct sufs_fidx_entry *idx = NULL;
    bool ret;

    if (s->file_type == SUFS_FILE_TYPE_REG)
        return false; 

    idx = (struct sufs_fidx_entry *)
            sufs_checker_offset_to_virt_addr(s->shadow_index_offset);

    while (idx->offset != 0)
    {
        if (sufs_is_norm_fidex(idx))
        {
            struct sufs_dir_entry *d = NULL;

            d = (struct sufs_dir_entry *)
                sufs_checker_offset_to_virt_addr(idx->offset);

            while (d->name_len != 0)
            {
                if (d->ino_num != SUFS_INODE_TOMBSTONE)
                {
                    /* Deleted inode */
                    if (!sufs_checker_chainhash_lookup(&h_inode, NULL, 0, 
                                                       d->ino_num))
                    {
                        ret = can_inode_deleted(d->ino_num, ino_num, tgid);
                        if (ret == false)
                            return true; 
                    }
                }

                d = (struct sufs_dir_entry *)((unsigned long)d + d->rec_len);
            }

            idx++;
        }
        else
        {
            idx = (struct sufs_fidx_entry *)
                sufs_checker_offset_to_virt_addr(idx->offset);
        }
    } 

    return false; 
}

static bool check_dir(int ino_num, unsigned long index_offset, int tgid, 
    int uid, int gid, bool* is_dir_relocated)
{
    struct sufs_fidx_entry *idx = NULL;

    if (index_offset == 0)
        goto out;

    if (is_dup_page(index_offset)) {
        CHECKER_DEBUG_PRINT("check_dir: index_offset %lx is duplicate page\n", index_offset);
        return false;
    }

    idx = (struct sufs_fidx_entry *) 
        sufs_checker_offset_to_virt_addr(index_offset);

    while (idx->offset != 0)
    {
        if (sufs_is_norm_fidex(idx))
        {
            struct sufs_dir_entry *d = NULL;

            if (is_dup_page(idx->offset)) {
                CHECKER_DEBUG_PRINT("check_dir: data page offset %lx is duplicate\n", idx->offset);
                return false;
            }

            d = (struct sufs_dir_entry *) 
                sufs_checker_offset_to_virt_addr(idx->offset);

            while (d->name_len != 0)
            {
                if (d->ino_num != SUFS_INODE_TOMBSTONE)
                {
                    if (!(is_dentry_valid(d, tgid, uid, gid))) {
                        CHECKER_DEBUG_PRINT("check_dir: dentry invalid (name=%s, ino_num=%d)\n", d->name, d->ino_num);
                        return false;
                    }

                    if (is_dup_name(d->name, d->name_len)) {
                        CHECKER_DEBUG_PRINT("check_dir: duplicate name '%s'\n", d->name);
                        return false; 
                    }
                    
                    struct sufs_shadow_inode *d_si = get_shadow_inode(d->ino_num);

                    if (d_si->parent != ino_num && d_si->file_type != SUFS_FILE_TYPE_NONE) {
                        // In this case, LibFS renamed a file (d->ino_num) from old parent (d_si->parent) to new parent (ino_num).
                        
                        // The old parent (d_si->parent) must be the owned one because, after verification succeeds, 
                        // we update `d_si->parent` to `ino_num`, which is treated as the ground truth.
                        // If the old parent is not the owned one, the file could be renamed without proper permission.
                        if (sufs_mapped_state[d_si->parent] != tgid) return false;

                        // The new parent (ino_num) must not be a descendant of the renamed file (d->ino_num).
                        // Otherwise, the renamed file is not guaranteed to be connected to the root via the new parent (ino_num)
                        // and forms a disconnected cycle.
                        if (d_si->file_type == SUFS_FILE_TYPE_DIR) {
                            *is_dir_relocated = true;
                            int curr = ino_num;
                            while (curr != SUFS_ROOT_INODE) {
                                if(curr == d->ino_num) return false;
                                struct sufs_shadow_inode *si = get_shadow_inode(curr);
                                curr = si->parent;
                            }
                        }
                    }
                }

                d = (struct sufs_dir_entry *) ((unsigned long) d + d->rec_len);
            }

            idx++;
        }
        else
        {
            idx = (struct sufs_fidx_entry *)
                sufs_checker_offset_to_virt_addr(idx->offset);

            if (is_dup_page(idx->offset)) {
                CHECKER_DEBUG_PRINT("check_dir: index page offset %lx is duplicate\n", idx->offset);
                return false;
            }
        }
    }

out:
    if (is_disconnected(ino_num, tgid)) {
        CHECKER_DEBUG_PRINT("check_dir: inode %d is disconnected\n", ino_num);
        return false;
    }
    return true;
}

static bool check_inode(int ino_num, char file_type, 
    unsigned long index_offset, int tgid, int uid, int gid, bool* is_dir_relocated)
{
    if (!is_inode_range_valid(ino_num)) {
        CHECKER_DEBUG_PRINT("check_inode: ino_num %d out of range\n", ino_num);
        return false;
    }

    if (ino_num == SUFS_ROOT_INODE)
    {
        struct sufs_shadow_inode * sinode = get_shadow_inode(ino_num);
        if (sinode->file_type != SUFS_FILE_TYPE_DIR) {
            CHECKER_DEBUG_PRINT("check_inode: root inode %d not a directory (file_type=%d)\n", ino_num, sinode->file_type);
            return false;
        }
    }

    if (file_type == SUFS_FILE_TYPE_REG)
    {
        bool ret = check_regular_files(index_offset, tgid);
        if (!ret) {
            CHECKER_DEBUG_PRINT("check_inode: regular file check failed (ino_num=%d, index_offset=%lx, tgid=%d)\n", ino_num, index_offset, tgid);
        }
        return ret;
    }
    else if (file_type == SUFS_FILE_TYPE_DIR)
    {
        bool ret = check_dir(ino_num, index_offset, tgid, uid, gid, is_dir_relocated);
        if (!ret) {
            CHECKER_DEBUG_PRINT("check_inode: directory check failed (ino_num=%d, index_offset=%lx, tgid=%d uid=%d, gid=%d)\n", ino_num, index_offset, tgid, uid, gid);
        }
        return ret;
    }
    else
    {
        /* Wrong file type */
        CHECKER_DEBUG_PRINT("check_inode: wrong file type %d for ino_num %d\n", file_type, ino_num);
        return false;
    }

}


int main(int argc, char *argv[])
{
    struct sufs_ioctl_checker_map_entry map_e;

    int fd = open(SUFS_DEV_PATH, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    if (ioctl(fd, SUFS_CMD_CHECKER_MAP, &map_e) == -1)
    {
        perror("ioctl");
        return 1;
    }

    max_block = map_e.max_block;

    sufs_inode_state = (int *) SUFS_STATE_ADDR;
    sufs_mapped_state = (int *)(SUFS_STATE_ADDR + SUFS_INODE_STATE_SIZE);
    sufs_page_state = (int *) 
        (SUFS_STATE_ADDR + SUFS_INODE_STATE_SIZE + SUFS_MAPPED_STATE_SIZE);

    sufs_checker_file_ring = 
        sufs_checker_ring_buffer_connect(SUFS_CHECKER_FILE_RING_ADDR);

    sufs_checker_res_ring = 
        sufs_checker_ring_buffer_connect(SUFS_CHECKER_RES_RING_ADDR);

    if (setgid(getgid()) == -1)
    {
        perror("setgid");
        return 1;
    }

    if (setuid(getuid()) == -1)
    {
        perror("setuid");
        return 1;
    }

    sufs_checker_chainhash_init(&h_page, FILE_HASH_SIZE, 0);
    sufs_checker_chainhash_init(&h_inode, FILE_DIR_SIZE, 0);
    sufs_checker_chainhash_init(&h_name, FILE_DIR_SIZE, 1);

    FILE *f = fopen("/tmp/checker_ready", "w");
    if (f) {
        fprintf(f, "ready\n");
        fclose(f);
    }

    while (1)
    {
        bool ret = false; 
        struct sufs_ioctl_checker_entry e;

        sufs_checker_sr_receive_request(sufs_checker_file_ring, &e);
#if 0
        printf("\nreceive request: ino_num=%d, file_type=%d, index_offset=%lx, tgid=%d, uid: %d, gid: %d\n", e.ino_num, e.file_type, e.index_offset, e.tgid, e.uid, e.gid);
#endif
        bool is_dir_relocated = false;

        ret = check_inode(e.ino_num, e.file_type, e.index_offset, e.tgid, 
                e.uid, e.gid, &is_dir_relocated);

        if (ret == true)
        {
            // If there is a relocated directory into current inode (e.ino_num),
            // then LibFS must hold the rename lease.
            // Inform kernel to pass only if LibFS holds global rename lease.
            if (is_dir_relocated) 
                e.ret = SUFS_CHECKER_PASS_ONLY_IF_RENAMED_RET_CODE;
            else 
                e.ret = SUFS_CHECKER_PASS_RET_CODE; 
        }
        else 
        {
            e.ret = 1;
        }

#if 0
        printf("return %d\n\n", e.ret);
#endif 

        sufs_checker_sr_send_request(sufs_checker_res_ring, &e);

        sufs_checker_chainhash_reinit(&h_page);
        sufs_checker_chainhash_reinit(&h_inode);
        sufs_checker_chainhash_reinit(&h_name);
    }

    return 0;
}
