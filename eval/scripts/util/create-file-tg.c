#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../arckfsplus/include/config.h"

#define MAX_PATH 4096

static inline long long sufs_libfs_rdtsc(void)
{
    unsigned long hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (lo | (hi << 32));
}

int main(int argc, char *argv[]) 
{
    char * dir = NULL; 
    char path[MAX_PATH];
    int counter = 0, repeat = 0, i = 0;
    unsigned long begin = 0, end = 0;

    if (argc != 3) 
    {
    }

    dir = argv[1];
    repeat = strtol(argv[2], NULL, 10);

    for (i = 0; i < repeat; i++)
    {
        snprintf(path, MAX_PATH, "%s/%d", dir, i);

        begin = sufs_libfs_rdtsc();

        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) 
        {
            return 1;
        }

        close(fd);

        end =  sufs_libfs_rdtsc();

        if(i == 10 || i == 100) {
            printf("creat-%d: %lf us\n", i, (double)(end-begin) / 3000);
        }
    }

    return 0;
}
