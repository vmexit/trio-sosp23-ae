#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "../include/libfs_config.h"
#include "../include/common_inode.h"
#include "syscall.h"
#include "mnode.h"
#include "filetable.h"
#include "proc.h"
#include "mfs.h"
#include "file.h"
#include "ialloc.h"
#include "util.h"
#include "cmd.h"
#include "journal.h"

static struct sufs_libfs_file_mnode* sufs_libfs_getfile(struct sufs_libfs_proc
        *proc, int fd)
{
    struct sufs_libfs_filetable *filetable = proc->ftable;
    return sufs_libfs_filetable_getfile(filetable, fd);
}


static int sufs_libfs_fdalloc(struct sufs_libfs_proc *proc,
        struct sufs_libfs_file_mnode *f, int omode)
{
    struct sufs_libfs_filetable *filetable = proc->ftable;
    if (!f)
        return -1;

    return sufs_libfs_filetable_allocfd(filetable, f, omode & O_ANYFD,
            omode & O_CLOEXEC);
}

off_t sufs_libfs_sys_lseek(struct sufs_libfs_proc *proc, int fd,
        off_t offset, int whence)
{
    struct sufs_libfs_file_mnode *f = sufs_libfs_getfile(proc, fd);

    if (!f)
        return -1;

    if (sufs_libfs_mnode_type(f->m) != SUFS_FILE_TYPE_REG)
        return -1; //ESPIPE

    if (whence == SEEK_CUR)
    {
        offset += f->off;
    }
    else if (whence == SEEK_END)
    {
        u64 size = sufs_libfs_mnode_file_size(f->m);
        offset += size;
    }

    if (offset < 0)
        return -1;

    f->off = offset;

    return offset;
}

int sufs_libfs_sys_fstat(struct sufs_libfs_proc *proc, int fd,
        struct stat *stat)
{
    struct sufs_libfs_file_mnode *f = sufs_libfs_getfile(proc, fd);
    if (!f)
        return -1;

    if (sufs_libfs_mnode_stat(f->m, stat) < 0)
        return -1;

    return 0;
}

int sufs_libfs_sys_lstat(struct sufs_libfs_proc *proc, char *path,
        struct stat *stat)
{
    struct sufs_libfs_mnode *m = NULL;

    m = sufs_libfs_namei(proc->cwd_m, path, 1);
    if (!m)
    {
        errno = ENOENT;
        return -1;
    }

    if (sufs_libfs_mnode_stat(m, stat) < 0)
        return -1;

    return 0;
}

int sufs_libfs_sys_close(struct sufs_libfs_proc *proc, int fd)
{
    struct sufs_libfs_file_mnode *f = sufs_libfs_getfile(proc, fd);
    if (!f)
        return -1;

    sufs_libfs_filetable_close(proc->ftable, fd);

    return 0;
}

int sufs_libfs_sys_link(struct sufs_libfs_proc *proc, char *old_path, 
                        char *new_path)
{
    return -1;
}

static struct sufs_libfs_mnode*
sufs_libfs_rename_dir_lookup(struct sufs_libfs_mnode *mnode, char *name)
{
    unsigned long ino = 0;

    if (strcmp(name, "..") == 0)
    {
        return sufs_libfs_mnode_array[mnode->parent_mnum];
    }

    sufs_libfs_chainhash_lookup(&mnode->data.dir_data.map_, name, SUFS_NAME_MAX,
            &ino, NULL);

    return sufs_libfs_mnode_array[ino];
}

void sufs_libfs_rename_complete_func(struct sufs_libfs_mnode *mdnew, 
        char * newname, struct sufs_libfs_mnode *mfold, 
        struct sufs_libfs_mnode *mfroadblock,
        struct sufs_libfs_ch_item * item)
{
    struct sufs_dir_entry * new_dir = NULL, * old_dir = NULL, 
                            * rb_dir = NULL;
    unsigned long journal_tail = 0;

    int name_len = strlen(newname) + 1;
    int cpu = 0;

    sufs_libfs_mnode_dir_entry_insert(mdnew, newname, name_len, mfold, 
                                        &new_dir);
                                        
    memcpy(&(new_dir->inode), mfold->inode, sizeof(struct sufs_inode));

    old_dir = container_of(mfold->inode, struct sufs_dir_entry, inode);

    if (mfroadblock)
    {
        rb_dir = container_of(mfroadblock->inode, 
            struct sufs_dir_entry, inode);
    }

    cpu = sufs_libfs_current_cpu();

    pthread_spin_lock(&sufs_libfs_journal_locks[cpu]);

    journal_tail = sufs_libfs_create_rename_transaction(cpu,
            &(new_dir->name_len), &(old_dir->ino_num), &(rb_dir->ino_num));

    new_dir->name_len = name_len;
    sufs_libfs_clwb_buffer(new_dir, 
            sizeof(struct sufs_dir_entry) + name_len, 0);

    old_dir->ino_num = SUFS_INODE_TOMBSTONE;

    sufs_libfs_clwb_buffer(&(old_dir->ino_num), 
            sizeof(old_dir->ino_num), 0);

    if (mfroadblock)
    {
        rb_dir->ino_num = SUFS_INODE_TOMBSTONE;

        sufs_libfs_clwb_buffer(&(rb_dir->ino_num), 
            sizeof(rb_dir->ino_num), 0);
    }

    sufs_libfs_sfence();

    sufs_libfs_commit_lite_transaction(cpu, journal_tail);

    pthread_spin_unlock(&sufs_libfs_journal_locks[cpu]);

    item->val2 = (unsigned long) new_dir;

    mfold->inode = &(new_dir->inode);
    mfold->parent_mnum = mdnew->ino_num;
}

int sufs_libfs_sys_rename(struct sufs_libfs_proc *proc, char *old_path,
        char *new_path)
{
    char oldname[SUFS_NAME_MAX], newname[SUFS_NAME_MAX];
    struct sufs_libfs_mnode *cwd_m = NULL, *mdold = NULL, *mdnew = NULL,
            *mfold = NULL, *mfroadblock = NULL;

    int ret = 0, cross_dir_rename = 0;

    int mfold_type = 0;

    cwd_m = proc->cwd_m;

    mdold = sufs_libfs_nameiparent(cwd_m, old_path, oldname, 1);
    if (!mdold)
        return -1;

    mdnew = sufs_libfs_nameiparent(cwd_m, new_path, newname, 1);
    if (!mdnew)
        return -1;

    if (strcmp(oldname, ".") == 0 || strcmp(oldname, "..") == 0
            || strcmp(newname, ".") == 0 || strcmp(newname, "..") == 0)
        return -1;


    if (sufs_libfs_map_file(mdold, 1) != 0)
        goto out_err_mdold;

    if (sufs_libfs_map_file(mdnew, 1) != 0)
        goto out;

    if (!(mfold = sufs_libfs_rename_dir_lookup(mdold, oldname)))
        goto out;

    mfold_type = sufs_libfs_mnode_type(mfold);


    if (mdold == mdnew && oldname == newname)
        return 0;

    mfroadblock = sufs_libfs_rename_dir_lookup(mdnew, newname);

    if (mfroadblock)
    {
        int mfroadblock_type = sufs_libfs_mnode_type(mfroadblock);

        /*
         * Directories can be renamed to directories; and non-directories can
         * be renamed to non-directories. No other combinations are allowed.
         */

        if (mfroadblock_type != mfold_type)
            return -1;
    }

    if (mfroadblock == mfold)
    {
        /*
         * If the old and new paths point to the same inode, POSIX specifies
         * that we return successfully with no further action.
         */
        ret = 0;
        goto out;
    }

    if (mdold != mdnew && mfold_type == SUFS_FILE_TYPE_DIR)
    {
        /* Loop avoidance: Abort if the source is
         * an ancestor of the destination. */
        struct sufs_libfs_mnode *md = mdnew;
        cross_dir_rename = 1; 
        sufs_libfs_cmd_acquire_rename_lease();

        while (1)
        {
            if (mfold == md)
                return -1;

            if (md->ino_num == SUFS_ROOT_INODE)
                break;

            md = sufs_libfs_rename_dir_lookup(md, "..");

        }
    }

    /* Perform the actual rename operation in hash table */
    if (sufs_libfs_mnode_dir_replace_from(mdnew, newname, mfroadblock, mdold, 
        oldname, mfold, mfold_type == SUFS_FILE_TYPE_DIR ? mfold : NULL, 
        sufs_libfs_rename_complete_func))
    {
        ret = 0;
    }
    else
    {
        ret = -1;
    }


out:
    if (cross_dir_rename)
    {
        unsigned long index_offset = 0; 

        if (mdnew->index_start)
        {
            index_offset = sufs_libfs_virt_addr_to_offset((unsigned long)   
                    mdnew->index_start);
        }

        sufs_libfs_cmd_commit(mdnew->ino_num, mdnew->type, index_offset);
        sufs_libfs_cmd_release_rename_lease();
    }

out_err_mdold:
    return ret;
}

static struct sufs_libfs_mnode* sufs_libfs_create(struct sufs_libfs_mnode *cwd,
        char *path, short type, unsigned int mode,
        unsigned int uid, unsigned int gid, bool excl, int * error)
{
    int inode = 0;
    char name[SUFS_NAME_MAX];

    struct sufs_libfs_mnode *md = sufs_libfs_nameiparent(cwd, path, name, 1);
    struct sufs_libfs_mnode *mf = NULL;
    struct sufs_libfs_mnode *ret = NULL;
    struct sufs_libfs_mnode *ret_mf = NULL;

    struct sufs_dir_entry * dir = NULL;
    int name_len = 0, insert_result = 0;

    if (!md)
    {
        return NULL;
    }

    inode = sufs_libfs_new_inode(&sufs_libfs_sb, sufs_libfs_current_cpu());

    if (inode <= 0)
    {
        if (error)
        {
            *error = ENOSPC;
        }

        return NULL;
    }

    mf = sufs_libfs_mfs_mnode_init(type, inode, md->ino_num, NULL);

    name_len = strlen(name) + 1;

    insert_result = sufs_libfs_mnode_dir_insert(md, name, name_len, 
            mf, &ret_mf, &dir);
    if (insert_result == SUFS_LIBFS_ERR_SUCCESS)
    {
        unsigned long start_flush_addr = 0, flush_len = 0;
        sufs_libfs_inode_init(&(dir->inode), type, mode, uid, gid, 0);

        mf->inode = &(dir->inode);
        memcpy(&(mf->shadow_inode), mf->inode, sizeof(struct sufs_inode));

        mf->index_start = NULL;
        mf->index_end = NULL;

        start_flush_addr = CACHE_ROUND_DOWN(dir);

        flush_len = CACHE_ROUND_UP(sizeof(struct sufs_dir_entry) + name_len);
        sufs_libfs_clwb_buffer((void *) (start_flush_addr + SUFS_CACHELINE), 
                flush_len - SUFS_CACHELINE, 0);

        sufs_libfs_sfence();

        dir->name_len = name_len;

        sufs_libfs_clwb_buffer((void *) start_flush_addr, SUFS_CACHELINE , 0);
        sufs_libfs_sfence();
        return mf;
    } 
    else if (insert_result == SUFS_LIBFS_ERR_ALREADY_EXIST)
    {
        if (excl)
        {
            if (error)
            {
                *error = EEXIST;
            }

            ret = NULL;
        }
        else if (ret_mf)
        {
            if (type != SUFS_FILE_TYPE_REG
                    || !(sufs_libfs_mnode_type(ret_mf) == SUFS_FILE_TYPE_REG)
                    || excl)
                ret =  NULL;

            ret = ret_mf;
        }
    }

    sufs_libfs_free_inode(&sufs_libfs_sb, inode);
    sufs_libfs_mnode_array[inode] = NULL;
    free(mf);

    return ret;
}

int sufs_libfs_sys_openat(struct sufs_libfs_proc *proc, int dirfd, char *path,
        int flags, int mode)
{
    struct sufs_libfs_mnode *cwd = NULL, *m = NULL;
    struct sufs_libfs_file_mnode *f = NULL;
    int rwmode = 0;
    int ret = 0;
    int err = 0;

    if (dirfd == AT_FDCWD)
    {
        cwd = proc->cwd_m;
    }
    else
    {
        struct sufs_libfs_file_mnode *fdirm =
                (struct sufs_libfs_file_mnode*) sufs_libfs_getfile(proc, dirfd);

        if (!fdirm)
            return -1;

        cwd = fdirm->m;
    }

    if (flags & O_CREAT)
        m = sufs_libfs_create(cwd, path, SUFS_FILE_TYPE_REG,
                mode, proc->uid, proc->gid, flags & O_EXCL, &err);
    else
        m = sufs_libfs_namei(cwd, path, 1);

    if (!m)
    {
        errno = err;
        return -1;
    }

    rwmode = flags & (O_RDONLY | O_WRONLY | O_RDWR);
    if ((sufs_libfs_mnode_type(m) == SUFS_FILE_TYPE_DIR) 
        && (rwmode != O_RDONLY))
        return -1;

    if ((ret = sufs_libfs_map_file(m, !(rwmode == O_RDONLY))) != 0)
    {
        return ret;
    }

    if ((sufs_libfs_mnode_type(m) == SUFS_FILE_TYPE_REG) && (flags & O_TRUNC))
    {
        if (sufs_libfs_mnode_file_size(m))
            sufs_libfs_mnode_file_truncate_zero(m);
    }

    f = sufs_libfs_file_mnode_init(m, !(rwmode == O_WRONLY), 
                                      !(rwmode == O_RDONLY),
                                      !!(flags & O_APPEND));


    return sufs_libfs_fdalloc(proc, f, flags);
}

int sufs_libfs_sys_unlink(struct sufs_libfs_proc *proc, char *path)
{
    char name[SUFS_NAME_MAX];
    struct sufs_libfs_mnode *md = NULL, *cwd_m = NULL, *mf = NULL;
    int mf_type = 0, mf_locked = 0;

    cwd_m = proc->cwd_m;

    md = sufs_libfs_nameiparent(cwd_m, path, name, 1);
    if (!md)
        return -1;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return -1;

    mf = sufs_libfs_mnode_dir_lookup(md, name, 1);
    if (!mf)
        return -1;

    mf_type = sufs_libfs_mnode_type(mf);

    if (mf_type == SUFS_FILE_TYPE_DIR)
    {
        if (!sufs_libfs_mnode_dir_kill(mf, &mf_locked))
        {
            if (mf_locked)
            {
                sufs_libfs_chainhash_release_all_locks(&mf->data.dir_data.map_);
                mf_locked = 0;
            }
            return -1;
        }
    }

    assert(sufs_libfs_mnode_dir_remove(md, name));

    if (mf_locked)
    {
        sufs_libfs_chainhash_release_all_locks(&mf->data.dir_data.map_);
        mf_locked = 0;
    }

    sufs_libfs_mnode_file_delete(mf);

    if (sufs_libfs_is_inode_allocated(mf->ino_num))
    {
        sufs_libfs_free_inode(&sufs_libfs_sb, mf->ino_num);
    } 

    sufs_libfs_mnode_array[mf->ino_num] = NULL;

    sufs_libfs_mnode_free(mf);

    return 0;
}

ssize_t sufs_libfs_sys_read(struct sufs_libfs_proc *proc, int fd, void *p, size_t n)
{
    struct sufs_libfs_file_mnode *f = NULL;
    ssize_t res = 0;

    f = sufs_libfs_getfile(proc, fd);

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return -1;
    }

    res = sufs_libfs_file_mnode_read(f, (char*) p, n);
    if (res < 0)
    {
        res = -1;
        goto out;
    }

out:
    return res;
}

ssize_t sufs_libfs_sys_pread(struct sufs_libfs_proc *proc, int fd, void *ubuf,
        size_t count, off_t offset)
{
    struct sufs_libfs_file_mnode *f = NULL;
    ssize_t r = 0;

    f = sufs_libfs_getfile(proc, fd);

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return -1;
    }

    r = sufs_libfs_file_mnode_pread(f, (char*) ubuf, count, offset);

    return r;
}

ssize_t sufs_libfs_sys_pwrite(struct sufs_libfs_proc *proc, int fd,
        void *ubuf, size_t count, off_t offset)
{
    struct sufs_libfs_file_mnode *f = NULL;
    ssize_t r = 0;

    f = sufs_libfs_getfile(proc, fd);

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return -1;
    }

    r = sufs_libfs_file_mnode_pwrite(f, (char*) ubuf, count, offset);

    return r;
}

ssize_t sufs_libfs_sys_write(struct sufs_libfs_proc *proc, int fd, void *p,
        size_t n)
{
    struct sufs_libfs_file_mnode *f = NULL;

    ssize_t res = 0;

    f = sufs_libfs_getfile(proc, fd);

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return -1;
    }

    res = sufs_libfs_file_mnode_write(f, (char*) p, n);

    return res;
}

int sufs_libfs_sys_mkdirat(struct sufs_libfs_proc *proc, int dirfd, char *path,
        mode_t mode)
{
    struct sufs_libfs_mnode *cwd = NULL;
    int err = 0;

    if (strcmp(path, "/") == 0)
    {
        errno = EEXIST;
        return -1;
    }

    if (dirfd == AT_FDCWD)
    {
        cwd = proc->cwd_m;
    }
    else
    {
        struct sufs_libfs_file_mnode *fdir = sufs_libfs_getfile(proc, dirfd);
        if (!fdir)
            return -1;

        cwd = fdir->m;
    }

    if (!sufs_libfs_create(cwd, path, SUFS_FILE_TYPE_DIR, mode,
            proc->uid, proc->gid, true, &err))
    {
        errno = err;
        return -1;
    }

    return 0;
}

int sufs_libfs_sys_chown(struct sufs_libfs_proc *proc, char * path,
        uid_t owner, gid_t group)
{
    struct sufs_libfs_mnode *m = NULL;
    unsigned long inode_offset = 0;

    m = sufs_libfs_namei(proc->cwd_m, path, 1);
    if (!m)
        return -1;

    inode_offset = sufs_libfs_virt_addr_to_offset((unsigned long) m->inode);

    return sufs_libfs_cmd_chown(m->ino_num, owner, group, inode_offset);
}

int sufs_libfs_sys_chmod(struct sufs_libfs_proc *proc, char * path,
        mode_t mode)
{
    struct sufs_libfs_mnode *m = NULL;
    unsigned long inode_offset = 0;

    m = sufs_libfs_namei(proc->cwd_m, path, 1);
    if (!m)
        return -1;

    inode_offset = sufs_libfs_virt_addr_to_offset((unsigned long) m->inode);

    return sufs_libfs_cmd_chmod(m->ino_num, mode, inode_offset);
}

int sufs_libfs_sys_ftruncate(struct sufs_libfs_proc *proc, int fd,
        off_t length)
{
    struct sufs_libfs_file_mnode *f = NULL;

    ssize_t res = 0;

    f = sufs_libfs_getfile(proc, fd);

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return -1;
    }

    res = sufs_libfs_file_mnode_truncate(f, length);

    return res;
}



int sufs_libfs_sys_chdir(struct sufs_libfs_proc *proc, char *path)
{
    return -1;
}

int sufs_libfs_sys_readdir(struct sufs_libfs_proc *proc, int dirfd,
        char *prevptr, char *nameptr)
{
    struct sufs_libfs_file_mnode *df = sufs_libfs_getfile(proc, dirfd);

    if (!df)
        return -1;

    if (sufs_libfs_mnode_type(df->m) != SUFS_FILE_TYPE_DIR)
        return -1;

    if (!sufs_libfs_mnode_dir_enumerate(df->m, prevptr, nameptr))
    {
        return 0;
    }

    return 1;
}

__ssize_t sufs_libfs_sys_getdents(struct sufs_libfs_proc *proc, int dirfd,
        void * buffer, size_t length)
{
    struct sufs_libfs_file_mnode *df = sufs_libfs_getfile(proc, dirfd);

    if (!df)
        return -1;

    if (sufs_libfs_mnode_type(df->m) != SUFS_FILE_TYPE_DIR)
        return -1;

    return sufs_libfs_mnode_dir_getdents(df->m, &(df->off), buffer, length);
}

void * sufs_libfs_sys_get_inode(struct sufs_libfs_proc *proc, int fd)
{
    struct sufs_libfs_file_mnode *f = NULL;

    f = sufs_libfs_getfile(proc, fd);

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return NULL;
    }

    if (f->m == NULL)
        return NULL;

    return f->m->inode; 
}

void *sufs_libfs_sys_get_dentry(struct sufs_libfs_proc *proc, int fd)
{
    struct sufs_libfs_file_mnode *f = NULL;

    f = sufs_libfs_getfile(proc, fd);

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return NULL;
    }

    if (f->m == NULL)
        return NULL;

    return container_of(f->m->inode, struct sufs_dir_entry, inode);
}

void *sufs_libfs_sys_get_inode_by_path(struct sufs_libfs_proc *proc, 
                                       char * path)
{
    struct sufs_libfs_mnode* m = NULL;

    m = sufs_libfs_namei(proc->cwd_m, path, 1);

    if (!m)
    {
        fprintf(stderr, "Cannot find file from path: %s\n", path);
        return NULL;
    }

    return m->inode;
}

void *sufs_libfs_sys_get_dentry_by_path(struct sufs_libfs_proc *proc, 
     char *path)
{
   struct sufs_libfs_mnode* m = NULL;

    m = sufs_libfs_namei(proc->cwd_m, path, 1);

    if (!m)
    {
        fprintf(stderr, "Cannot find file from path: %s\n", path);
        return NULL;
    }

    return container_of(m->inode, struct sufs_dir_entry, inode);
}

int sufs_libfs_sys_commit(struct sufs_libfs_proc *proc, int fd)
{
    struct sufs_libfs_file_mnode *f = NULL;
    unsigned long index_offset = 0;

    f = sufs_libfs_getfile(proc, fd);
    

    if (!f)
    {
        fprintf(stderr, "Cannot find file from fd: %d\n", fd);
        return 0;
    }

    if (f->m == NULL)
        return 0;

    if (f->m->index_start)
    {
        index_offset = sufs_libfs_virt_addr_to_offset((unsigned long)
                f->m->index_start);
    }

    return sufs_libfs_cmd_commit(f->m->ino_num, f->m->shadow_inode.file_type,
                                index_offset);
}

int sufs_libfs_sys_commit_by_path(struct sufs_libfs_proc *proc, char * path)
{
    struct sufs_libfs_mnode* m = NULL;
    struct sufs_inode *inode = NULL;

    m = sufs_libfs_namei(proc->cwd_m, path, 1);

    if (!m)
    {
        fprintf(stderr, "Cannot find file: %s\n", path);
        return 0;
    }

    inode = m->inode;

    if (inode == NULL)
        return 0;

    return sufs_libfs_cmd_commit(m->ino_num, inode->file_type,
                                 inode->offset);
}