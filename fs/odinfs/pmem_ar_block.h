/*
 * pmem_ar_block.h
 *
 *  Created on: Jul 14, 2021
 *      Author: dzhou
 */

#ifndef SPMFS_PMEM_AR_BLOCK_H_
#define SPMFS_PMEM_AR_BLOCK_H_

#include "pmem_ar.h"

extern struct pmem_ar_dev pmem_ar_dev;

struct pmem_ar_dev {
	/* Linux kernel block device fields */
	struct gendisk *gd;
	struct request_queue *queue;

	/* number of pmem devices in the pmem array*/
	int elem_num;

	/* bdevs of pmem devices in the pmem array */
	struct block_device *bdevs[PMFS_PMEM_AR_MAX_DEVICE];

	struct dax_device *dax_dev[PMFS_PMEM_AR_MAX_DEVICE];

	unsigned long start_virt_addr[PMFS_PMEM_AR_MAX_DEVICE];
	unsigned long size_in_bytes[PMFS_PMEM_AR_MAX_DEVICE];
};

int spmfs_pmem_ar_init_block(void);
void spmfs_pmem_ar_exit_block(void);

void spmfs_put_pmem_ar(void);

#endif /* SPMFS_PMEM_AR_BLOCK_H_ */
