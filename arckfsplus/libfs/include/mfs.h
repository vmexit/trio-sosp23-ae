#ifndef SUFS_LIBFS_MFS_H_
#define SUFS_LIBFS_MFS_H_

#include <stdatomic.h>

#include "../../include/libfs_config.h"
#include "types.h"
#include "balloc.h"
#include "ialloc.h"
#include "proc.h"

extern struct sufs_libfs_mnode *sufs_libfs_root_dir;

extern atomic_char *sufs_libfs_inode_mapped_attr;

extern atomic_char * sufs_libfs_inode_has_index;


extern pthread_spinlock_t * sufs_libfs_inode_map_lock;

struct sufs_libfs_mnode* sufs_libfs_namex(struct sufs_libfs_mnode *cwd,
        char *path, bool nameiparent, char *name, int map);

static inline struct sufs_libfs_mnode* sufs_libfs_namei(
        struct sufs_libfs_mnode *cwd, char *path, int map)
{
    char buf[SUFS_NAME_MAX];
    return sufs_libfs_namex(cwd, path, false, buf, map);
}

static inline struct sufs_libfs_mnode* sufs_libfs_nameiparent(
        struct sufs_libfs_mnode *cwd, char *path, char *buf, int map)
{
    return sufs_libfs_namex(cwd, path, true, buf, map);
}

static inline int sufs_libfs_file_should_release(struct sufs_libfs_mnode *m)
{
        int wanted = sufs_libfs_bm_test_bit((atomic_char*)              
                SUFS_WANTED_RING_ADDR, m->ino_num); 

        unsigned long * lease_time = 
                (unsigned long *) SUFS_DDL_RING_ADDR;

        return (wanted && (sufs_libfs_rdtsc() > lease_time[m->ino_num]));
}

static inline int sufs_libfs_file_is_real_mapped(struct sufs_libfs_mnode *m)
{
    return ( sufs_libfs_bm_test_bit((atomic_char*) SUFS_MAPPED_RING_ADDR, 
             m->ino_num)); 
}

static inline int sufs_libfs_file_is_mapped(struct sufs_libfs_mnode *m)
{
    return (!m || sufs_libfs_is_inode_allocated(m->ino_num) ||
           (sufs_libfs_bm_test_bit((atomic_char*) SUFS_MAPPED_RING_ADDR, 
             m->ino_num) && 
             sufs_libfs_bm_test_bit(sufs_libfs_inode_has_index, m->ino_num)
            ));
}

static inline void sufs_libfs_file_set_writable(struct sufs_libfs_mnode * m)
{
    sufs_libfs_bm_set_bit(sufs_libfs_inode_mapped_attr, m->ino_num);
}

static inline int sufs_libfs_file_mapped_writable(struct sufs_libfs_mnode *m)
{
    return (sufs_libfs_is_inode_allocated(m->ino_num) ||
            sufs_libfs_bm_test_bit(sufs_libfs_inode_mapped_attr, m->ino_num));
}

static inline void sufs_libfs_lock_file_mapping(struct sufs_libfs_mnode * m)
{
    long index = m->ino_num % SUFS_LIBFS_FILE_MAP_LOCK_SIZE;
    pthread_spin_lock(&(sufs_libfs_inode_map_lock[index]));
}

static inline void sufs_libfs_unlock_file_mapping(struct sufs_libfs_mnode * m)
{
    long index = m->ino_num % SUFS_LIBFS_FILE_MAP_LOCK_SIZE;
    pthread_spin_unlock(&(sufs_libfs_inode_map_lock[index]));
}

void sufs_libfs_mfs_init(void);

void sufs_libfs_mfs_fini(void);

void sufs_libfs_mfs_add_mapped_inode(int ino);
void sufs_libfs_mfs_unmap_mapped_inode(void);

void sufs_libfs_fs_init();

void sufs_libfs_fs_fini(void);

s64 sufs_libfs_readm(struct sufs_libfs_mnode *m, char *buf, u64 start,
        u64 nbytes);

s64 sufs_libfs_writem(struct sufs_libfs_mnode *m, char *buf, u64 start,
        u64 nbytes);

void sufs_libfs_flush_file_index(struct sufs_fidx_entry * start,
        struct sufs_fidx_entry * end);

int sufs_libfs_truncatem(struct sufs_libfs_mnode *m, off_t length);

int sufs_libfs_map_file(struct sufs_libfs_mnode *m, int writable);

int sufs_libfs_do_map_file(struct sufs_libfs_mnode *m, int writable);

void sufs_libfs_file_build_index(struct sufs_libfs_mnode *m);

int sufs_libfs_sys_unmap_by_path(struct sufs_libfs_proc *proc, char * path);

#endif /* SUFS_MFS_H_ */
