#ifndef SUFS_LIBFS_FILETABLE_H_
#define SUFS_LIBFS_FILETABLE_H_

#include <stdint.h>
#include <stdbool.h>

#include "../../include/libfs_config.h"


#define O_ANYFD 0x1000

#define SUFS_LIBFS_FILETABLE_CPUSHIFT 16
#define SUFS_LIBFS_FILETABLE_FDMASK ((1 << SUFS_LIBFS_FILETABLE_CPUSHIFT) - 1)

struct sufs_libfs_fdinfo
{
        uintptr_t data_;
};

static inline void sufs_libfs_fdinfo_init(struct sufs_libfs_fdinfo *info,
        void *fp, bool cloexec, bool locked)
{
    info->data_ = ((uintptr_t) fp | (uintptr_t) cloexec
            | ((uintptr_t) locked << 1));
}

static inline struct sufs_libfs_file_mnode*
sufs_libfs_fdinfo_getfile(struct sufs_libfs_fdinfo *info)
{
    return (struct sufs_libfs_file_mnode*) (info->data_ & ~3);
}

static inline bool sufs_fdinfo_get_cloexec(struct sufs_libfs_fdinfo *info)
{
    return (info->data_ & 1);
}

static inline bool sufs_libfs_fdinfo_get_locked(struct sufs_libfs_fdinfo *info)
{
    return (info->data_ & 2);
}

static inline struct sufs_libfs_fdinfo sufs_libfs_fdinfo_with_locked(
        struct sufs_libfs_fdinfo *info, bool locked)
{
    struct sufs_libfs_fdinfo ret;

    ret.data_ = (info->data_ & ~2) | ((uintptr_t) locked << 1);

    return ret;
}

struct sufs_libfs_filetable
{
        struct sufs_libfs_fdinfo info_[SUFS_MAX_CPU][SUFS_LIBFS_MAX_FD];
        bool cloexec_[SUFS_MAX_CPU][SUFS_LIBFS_MAX_FD];
};

struct sufs_libfs_fdinfo sufs_libfs_filetable_lock_fdinfo(
        struct sufs_libfs_fdinfo *infop);

struct sufs_libfs_file_mnode* sufs_libfs_filetable_getfile(
        struct sufs_libfs_filetable *ft, int fd);

int sufs_libfs_filetable_allocfd(struct sufs_libfs_filetable *ft,
        struct sufs_libfs_file_mnode *f, bool percpu, bool cloexec);

void sufs_libfs_filetable_close(struct sufs_libfs_filetable *ft, int fd);

void sufs_libfs_filetable_init(struct sufs_libfs_filetable *ft);

#endif /* SUFS_FILETABLE_H_ */
