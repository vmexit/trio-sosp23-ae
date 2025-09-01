#ifndef SUFS_KFS_DEV_DAX_H_
#define SUFS_KFS_DEV_DAX_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dax.h>
#include "../include/kfs_config.h"

extern int pm_nr;

/*
 * For each dax_dev device, this struct records the
 * starting virtual address in the kernel and its size
 */
struct sufs_dev_arr
{
    int num;
    unsigned long start_virt_addr[SUFS_PM_MAX_INS];
    unsigned long size_in_bytes[SUFS_PM_MAX_INS];
};

int sufs_init_dev(int num);


struct dax_device *inode_dax(struct inode *inode);

#endif /* SUFS_KFS_DEV_DAX_H_ */
