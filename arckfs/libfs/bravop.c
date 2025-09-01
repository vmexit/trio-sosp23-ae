
#include <unistd.h>
#include <stdio.h>

#include "../include/libfs_config.h"
#include "bravop.h"

long sufs_libfs_ncores = 0;

void sufs_libfs_init_num_cores(void)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n == -1) 
    {
        abort();
    }   
    
    sufs_libfs_ncores = n; 
}

