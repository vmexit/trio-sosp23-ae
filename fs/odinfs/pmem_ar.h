/*
 * pmem_ar.h
 *
 *  Created on: Jul 13, 2021
 *      Author: dzhou
 */

#ifndef SPMFS_PMEM_AR_H_
#define SPMFS_PMEM_AR_H_

#define SPMFS_MAJOR 263
#define SPMFS_DEV_NAME "pmem_ar"
#define SPMFS_DEV_INSTANCE_NAME "pmem_ar0"
#define SPMFS_PMEM_AR_NUM 1

#define PMFS_PMEM_AR_MAX_DEVICE 8

struct pmem_arg_info {
	/* number of devices to add*/
	int num;

	/* path to these devices */
	char *paths[PMFS_PMEM_AR_MAX_DEVICE];
};

#define SPMFS_PMEM_AR_CMD_CREATE 0
#define SPMFS_PMEM_AR_CMD_DELETE 1

#endif /* SPMFS_PMEM_AR_H_ */
