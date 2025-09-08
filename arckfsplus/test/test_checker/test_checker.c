#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#include "fault_num.h"
#include "libfs_call.h"
#include "../../include/common_inode.h"

#define testing_dir    "/sufs/test/"
#define testing_file_A "/sufs/test/foo"
#define testing_file_B "/sufs/test/bar"

static short gen_random_short_ne(short a)
{
    short ret = 0;
    do
    {
        ret = random();
    } while (ret == a);

    return ret;
}

static char gen_random_char_ne(char a)
{
    char ret = 0;
    do
    {
        ret = random();
    } while (ret == a);

    return ret;
}

static unsigned int gen_random_uint_ne(unsigned int a)
{
    unsigned int ret = 0;
    do
    {
        ret = random();
    } while (ret == a);

    return ret;
}

static unsigned char gen_random_uchar_range(unsigned char begin, 
    unsigned char end)
{
    unsigned char ret = 0;
    do
    {
        ret = random();
    } while (ret < begin || ret > end);

    return ret;
}

static unsigned long gen_random_ulong(void)
{
    unsigned long ret = 0;
    size_t bits = sizeof(unsigned long) * 8;
    size_t step = 31; // random() returns 31 bits
    size_t filled = 0;

    while (filled < bits) {
        ret |= ((unsigned long)random()) << filled;
        filled += step;
    }
    return ret;
}

static long gen_random_long_range(long begin, long end)
{
    unsigned long ret = 0;
    do
    {
        ret = gen_random_ulong();
    } while (ret < begin || ret > end);

    return ret;
}

static void must_write(int fd, void * buf, unsigned long size)
{
    int already_write = 0;

    while (already_write != size)
    {
        int ret = 0;
        void * new_addr = (void *) ((unsigned long) buf + already_write);
        ret = write(fd, new_addr, size - already_write);

        if (ret == -1)
        {
            fprintf(stderr, "Cannot write to fd: %d, buf: %lx, size: %lu\n",
                    fd, (unsigned long) buf, size);

            fprintf(stderr, "reason: %s\n", strerror(errno));
            exit(1);

        }

        already_write += ret;
    }
}

static struct sufs_dir_entry * get_file_direntry(char *pdir, char *file)
{
    int fd = 0;
    struct sufs_dir_entry * dir = NULL;

    if ( (mkdir(pdir, 0777) < 0) && (errno != EEXIST) )
        return NULL;

    fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0777);

    if (fd < 0)
    {
        return NULL;
    }

    dir = sufs_libfs_getdentry(fd);

    close(fd);

    return dir; 
}

static struct sufs_inode *get_file_inode(char *pdir, char *file, int size)
{
    int fd = 0;
    struct sufs_inode *inode = NULL;

    if ( (mkdir(pdir, 0777) < 0) && (errno != EEXIST) )
    {
        return NULL;
    }

    fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0777);
    if (fd < 0)
    { 
        return NULL;
    }

    if (size > 0)
    {
        char * buf = malloc(size); 
        must_write(fd, buf, size); 
    }

    inode = sufs_libfs_getinode(fd);

    close(fd);

    return inode;
}

static inline struct sufs_fidx_entry * 
get_file_first_idx_entry(struct sufs_inode * i)
{
    struct sufs_fidx_entry * ret = (struct sufs_fidx_entry *) 
        (((unsigned long) i->offset) + SUFS_MOUNT_ADDR);

    return ret;
}

int main(int argc, char *argv[])
{
    int fault_num = 0, fault_num2 = 0, ret = 0, err_fa = 0, 
        err_fb = 0, err_d = 0, check_a = 0, check_b = 0, check_d = 0, has_b = 0;

    srandom((unsigned int) time(NULL));

    if (argc != 2 && argc != 3)
    {
        printf("Usage: %s fault_num [fault_num]\n", argv[0]);
        exit(1);
    }

    fault_num = atol(argv[1]);

    if (fault_num < FAULT_NUM_MIN || fault_num > FAULT_NUM_MAX)
    {
        fprintf(stderr, "Error fault_num: %d\n", fault_num);
        return 1;
    }

    if (argc == 3)
    {
        fault_num2 = atol(argv[2]);

        if (fault_num2 < FAULT_NUM_MIN || fault_num2 > FAULT_NUM_MAX)
        {
            fprintf(stderr, "Error fault_num: %d\n", fault_num);
            return 1;
        }
    }

fault_inject:
    switch (fault_num)
    {
        case FAULT_NUM_NAME_LEN:
        {
            struct sufs_dir_entry *dir = NULL;
            dir = get_file_direntry(testing_dir, testing_file_A);

            dir->name_len = gen_random_short_ne(3);
            err_d = 1;

            break;
        }

        case FAULT_NUM_REC_LEN:
        {
            struct sufs_dir_entry *dir = NULL;
            dir = get_file_direntry(testing_dir, testing_file_A);

            dir->rec_len = gen_random_short_ne(dir->rec_len);
            err_d = 1;

            break;
        }

        case FAULT_NUM_INV_NAME:
        {
            unsigned char len = 0, fault = 0;
            struct sufs_dir_entry *dir = NULL;

            dir = get_file_direntry(testing_dir, testing_file_A);

            fault = gen_random_uchar_range(0, FAULT_NUM_NAME_FAULT_NULL);

            if (fault == FAULT_NUM_NAME_FAULT_SLASH)
            {
                len = gen_random_uchar_range(0, dir->name_len - 1);
                dir->name[len] = '/';
            }
            else if (fault == FAULT_NUM_NAME_FAULT_DOT)
            {
                dir->name_len = 1; 
                strcpy(dir->name, ".");
            }
            else if (fault == FAULT_NUM_NAME_FAULT_DOT_DOT)
            {
                dir->name_len = 2;
                strcpy(dir->name, "..");
            }
            else
            {
                dir->name_len = 0; 
                dir->name[0] = '0';
            }

            err_d = 1;

            break;
        }

        case FAULT_NUM_INODE_REP: 
        {
            struct sufs_dir_entry *dir1 = NULL;
            struct sufs_dir_entry *dir2 = NULL;

            dir1 = get_file_direntry(testing_dir, testing_file_A);
            dir2 = get_file_direntry(testing_dir, testing_file_B);

            dir2->ino_num = dir1->ino_num;

            has_b = 1;

            err_d = 1;

            break; 
        }

        case FAULT_NUM_INODE_OUT:
        {
            struct sufs_dir_entry *dir1 = NULL;
            struct sufs_dir_entry *dir2 = NULL;

            dir1 = get_file_direntry(testing_dir, testing_file_A);
            dir2 = get_file_direntry(testing_dir, testing_file_B);

            has_b = 1;

            dir2->ino_num = 0; 

            err_d = 1;
            break;
        }

        case FAULT_NUM_SAME_NAME:
        {
            struct sufs_dir_entry *dir1 = NULL;
            struct sufs_dir_entry *dir2 = NULL;

            dir1 = get_file_direntry(testing_dir, testing_file_A);
            dir2 = get_file_direntry(testing_dir, testing_file_B);

            has_b = 1;

            strcpy(dir1->name, dir2->name);
            dir1->name_len = dir2->name_len;

            err_d = 1;

            break;
        }

        case FAULT_NUM_FILE_TYPE:
        {
            struct sufs_inode * inode = NULL;

            inode = get_file_inode(testing_dir, testing_file_A, 0);

            if (inode == NULL)
                return 0;

            inode->file_type = gen_random_uint_ne(inode->file_type);

            err_d = 1;

            break;
        }

        case FAULT_NUM_FILE_MODE:
        {
            struct sufs_inode *inode = NULL;
            inode = get_file_inode(testing_dir, testing_file_A, 0);

            inode->mode = 0777 + 10;

            err_d = 1;

            break; 
        }

        case FAULT_NUM_FILE_UID:
        {
            struct sufs_inode *inode = NULL;
            inode = get_file_inode(testing_dir, testing_file_A, 0);

            inode->uid = gen_random_uint_ne(inode->uid);

            err_d = 1;

            break;
        }

        case FAULT_NUM_FILE_GID:
        {
            struct sufs_inode *inode = NULL;
            inode = get_file_inode(testing_dir, testing_file_A, 0);

            inode->gid = gen_random_uint_ne(inode->gid);

            err_d = 1;

            break;
        }

        case FAULT_NUM_FILE_ATIME:
        {
            struct sufs_inode *inode = NULL;
            inode = get_file_inode(testing_dir, testing_file_A, 0);
            inode->atime = gen_random_long_range(10737360001ll, LONG_MAX);

            err_d = 1;

            break; 
        }

        case FAULT_NUM_FILE_CTIME:
        {
            struct sufs_inode *inode = NULL;
            inode = get_file_inode(testing_dir, testing_file_A, 0);
            inode->ctime = gen_random_long_range(10737360001ll, LONG_MAX);

            err_d = 1;

            break;
        }

        case FAULT_NUM_FILE_MTIME:
        {
            struct sufs_inode *inode = NULL;
            inode = get_file_inode(testing_dir, testing_file_A, 0);
            inode->ctime = gen_random_long_range(10737360001ll, LONG_MAX);

            err_d = 1;

            break;
        }

        case FAULT_NUM_IDX_PAGE_REP:
        {
            struct sufs_inode * inode1 = NULL;
            struct sufs_fidx_entry *idx1 = NULL;

            inode1 = get_file_inode(testing_dir, testing_file_A, 8192);
            idx1 = get_file_first_idx_entry(inode1);
            
            idx1->offset = inode1->offset; 

            err_fa = 1;
            break;
        }

        case FAULT_NUM_IDX_PAGE_OUT:
        {
            struct sufs_inode *inode1 = NULL;
            
            inode1 = get_file_inode(testing_dir, testing_file_A, 4096);

            inode1->offset = -1;

            err_fa = 1;
            break;
        }

        case FAULT_NUM_DATA_PAGE_REP:
        {
            struct sufs_inode *inode1 = NULL;

            struct sufs_fidx_entry *idx1 = NULL;
            struct sufs_fidx_entry *idx2 = NULL;

            inode1 = get_file_inode(testing_dir, testing_file_A, 8192);

            idx1 = get_file_first_idx_entry(inode1);
            idx2 = idx1 + 1;
    
            idx2->offset = idx1->offset; 

            err_fa = 1;
            break;
        }

        case FAULT_NUM_DATA_PAGE_OUT:
        {
            struct sufs_inode *inode1 = NULL;
            struct sufs_fidx_entry *idx1 = NULL;

            inode1 = get_file_inode(testing_dir, testing_file_A, 4096);

            idx1 = get_file_first_idx_entry(inode1);
            idx1->offset = -1;

            err_fa = 1;
            break;
        }

        case FAULT_NUM_DIR_DISCONNECT:
        {
            mkdir("/sufs/test", 0777);
            mkdir("/sufs/test/1", 0777);
            ret = sufs_unmap_by_path("/sufs/test/1");

            if (ret == 0)
            {
                fprintf(stderr, "Fail to detect error!\n");
                return 1;
            }

            printf("Detection passed!\n"); 
            return 0;
        }

        case FAULT_NUM_DIR_DISCONNECTS2:
        {
            struct sufs_inode *inode = NULL;

            mkdir("/sufs/test", 0777);
            mkdir("/sufs/test/1", 0777);
            mkdir("/sufs/test/1/2", 0777); 

            ret = sufs_unmap_by_path("/sufs/");
            if (ret != 0)
            {
                fprintf(stderr, "Correct unmap #1 failed!\n");
                return 1;
            }

            ret = sufs_unmap_by_path("/sufs/test/");
            if (ret != 0)
            {
                fprintf(stderr, "Correct unmap #2 failed!\n");
                return 1;
            }

            ret = sufs_unmap_by_path("/sufs/test/1");
            if (ret != 0)
            {
                fprintf(stderr, "Correct unmap #3 failed!\n");
                return 1;
            }

            ret = sufs_unmap_by_path("/sufs/test/1/2");
            if (ret != 0)
            {
                fprintf(stderr, "Correct unmap #4 failed!\n");
                return 1;
            }

            printf("Detection passed!\n"); 
            return 0;
        }


        default:
            fprintf(stderr, "Fault num: %d not handled!\n", fault_num);
            return 1;
    }

    if (argc == 3)
    {
        fault_num = fault_num2;
        argc = 0; 
        goto fault_inject;
    }

    if (has_b)
    {
        check_b = sufs_unmap_by_path(testing_file_B);
    }

    check_a = sufs_unmap_by_path(testing_file_A);
    check_d = sufs_unmap_by_path(testing_dir);

    if (err_fa && check_a == 0)
    {
        fprintf(stderr, "Fail to detect error!\n");
        return 1;
    }

    if (err_fb && check_b == 0)
    {
        fprintf(stderr, "Fail to detect error!\n");
        return 1;
    }

    if (err_d && check_d == 0) 
    {
        fprintf(stderr, "Fail to detect error!\n");
        return 1;
    }
    
    printf("Detection passed!\n"); 
    return 0; 
}
