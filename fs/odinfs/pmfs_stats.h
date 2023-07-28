/*
 * pmfs_stats.h
 *
 *  Created on: Sep 13, 2021
 *      Author: dzhou
 */

#ifndef __PMFS_STATS_H_
#define __PMFS_STATS_H_

#include <asm/msr.h>
#include <linux/cpu.h>

#include "pmfs_config.h"

extern atomic64_t fsync_pages;

static inline unsigned long pmfs_timing_start(void)
{
	unsigned long rax, rdx;
	__asm__ __volatile__("rdtscp\n" : "=a"(rax), "=d"(rdx) : : "%ecx");
	return (rdx << 32) + rax;
}

static inline unsigned long pmfs_timing_end(void)
{
	unsigned long rax, rdx;
	__asm__ __volatile__("rdtscp\n" : "=a"(rax), "=d"(rdx) : : "%ecx");
	return (rdx << 32) + rax;
}

enum timing_category {
	create_t,
	unlink_t,
	readdir_t,
	xip_read_t,
	xip_write_t,
	xip_write_fast_t,
	internal_write_t,
	memcpy_r_t,
	memcpy_w_t,
	alloc_blocks_t,
	new_trans_t,
	add_log_t,
	commit_trans_t,
	mmap_fault_t,
	fsync_t,
	free_tree_t,
	evict_inode_t,
	recovery_t,

	get_block_r_t,
	do_delegation_r_t,
	pre_fault_r_t,
	send_request_r_t,
	fini_delegation_r_t,

	get_block_w_t,
	do_delegation_w_t,
	pre_fault_w_t,
	send_request_w_t,
	fini_delegation_w_t,

	agent_receive_request_t,

	agent_addr_trans_r_t,
	agent_memcpy_r_t,

	agent_addr_trans_w_t,
	agent_memcpy_w_t,

	free_inode_t,
	create_commit_trans_t,
	add_nondir_t,
	new_inode_t,
	create_new_trans_t,

	TIMING_NUM,
};

extern const char *Timingstring_pmfs[TIMING_NUM];

extern unsigned long Timingstats_pmfs[TIMING_NUM];
DECLARE_PER_CPU(unsigned long[TIMING_NUM], Timingstats_percpu_pmfs);

extern unsigned long Countstats_pmfs[TIMING_NUM];
DECLARE_PER_CPU(unsigned long[TIMING_NUM], Countstats_percpu_pmfs);

extern int measure_timing_pmfs;

typedef unsigned long timing_t;

#if PMFS_MEASURE_TIME

#define increase_fsync_pages_count() { atomic64_inc(&fsync_pages); }

#define PMFS_DEFINE_TIMING_VAR(name) timing_t name

#define PMFS_START_TIMING(name, start)                                         \
	do {                                                                   \
		if (measure_timing_pmfs) {                                     \
			barrier();                                             \
			start = pmfs_timing_start();                           \
			barrier();                                             \
		}                                                              \
	} while (0)

#define PMFS_END_TIMING(name, start)                                           \
	do {                                                                   \
		if (measure_timing_pmfs) {                                     \
			timing_t end;                                          \
			barrier();                                             \
			end = pmfs_timing_end();                               \
			barrier();                                             \
			__this_cpu_add(Timingstats_percpu_pmfs[name],          \
				       (end - start));                         \
		}                                                              \
		__this_cpu_add(Countstats_percpu_pmfs[name], 1);               \
	} while (0)
#else

#define increase_fsync_pages_count() {}

#define PMFS_DEFINE_TIMING_VAR(name)

#define PMFS_START_TIMING(name, start)

#define PMFS_END_TIMING(name, start)
#endif

#endif /* __PMFS_STATS_H_ */
