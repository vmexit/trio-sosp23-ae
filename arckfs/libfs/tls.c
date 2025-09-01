#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>


#include "../include/libfs_config.h"
#include "tls.h"
#include "util.h"
#include "compiler.h"

struct sufs_libfs_tls sufs_libfs_tls_data[SUFS_MAX_THREADS] __mpalign__;
__thread int sufs_libfs_my_thread = -1;

int sufs_libfs_btid = 0;

void sufs_libfs_tls_init(void)
{
    sufs_libfs_btid = sufs_libfs_gettid();

    memset(sufs_libfs_tls_data, 0, sizeof(sufs_libfs_tls_data));
}


