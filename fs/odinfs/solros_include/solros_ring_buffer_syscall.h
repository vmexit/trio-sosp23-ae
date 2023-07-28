#ifndef _RING_BUFFER_SYSCALL_H_
#define _RING_BUFFER_SYSCALL_H_

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <string.h>


/* TODO: add macro to make this not compile with the kernel setup */

/*
 * For file system-related system calls, we need to invoke them without using
 * the functions in glibc, so that these system calls are not intercepted by
 * the libFS
 *
 */

static inline int sys_shm_open_raw(const char *name, int oflag, mode_t mode)
{
    char tmp[PATH_MAX];
    strncpy(tmp, "/dev/shm", PATH_MAX);
    strcat(tmp, name);

    oflag |= O_NOFOLLOW | O_CLOEXEC;

    return syscall(SYS_open, tmp, oflag, mode);
}

static inline int sys_shm_unlink_raw(const char *name)
{
    char tmp[PATH_MAX];
    strncpy(tmp, "/dev/shm/", PATH_MAX);
    strcat(tmp, name);

    return syscall(SYS_unlink, tmp);
}

static inline int sys_ftruncate_raw(int fd, off_t length)
{
    return syscall(SYS_ftruncate, fd, length);
}


static inline int sys_close_raw(int fd)
{
    return syscall(SYS_close, fd);
}




#endif /* _RING_BUFFER_SYSCALL_H_ */
