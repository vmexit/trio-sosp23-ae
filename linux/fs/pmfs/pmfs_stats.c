#include "pmfs.h"

const char *Timingstring_pmfs[TIMING_NUM] = 
{
	"create",
	"unlink",
	"readdir",
	"xip_read",
	"xip_write",
	"xip_write_fast",
	"internal_write",
	"memcpy_read",
	"memcpy_write",
	"alloc_blocks",
	"new_trans",
	"add_logentry",
	"commit_trans",
	"mmap_fault",
	"fsync",
	"free_tree",
	"evict_inode",
	"recovery",
};

unsigned long long Timingstats_pmfs[TIMING_NUM];
u64 Countstats_pmfs[TIMING_NUM];

atomic64_t fsync_pages = ATOMIC_INIT(0);

void pmfs_print_IO_stats(void)
{
	printk("=========== PMFS I/O stats ===========\n");
	printk("Fsync %lld pages\n", atomic64_read(&fsync_pages));
}

void pmfs_print_timing_stats(void)
{
	int i;

	printk("======== PMFS kernel timing stats ========\n");
	for (i = 0; i < TIMING_NUM; i++) {
		if (measure_timing_pmfs || Timingstats_pmfs[i]) {
			printk("%s: count %llu, timing %llu, average %llu\n",
				Timingstring_pmfs[i],
				Countstats_pmfs[i],
				Timingstats_pmfs[i],
				Countstats_pmfs[i] ?
				Timingstats_pmfs[i] / Countstats_pmfs[i] : 0);
		} else {
			printk("%s: count %llu\n",
				Timingstring_pmfs[i],
				Countstats_pmfs[i]);
		}
	}

	pmfs_print_IO_stats();
}

void pmfs_clear_stats(void)
{
	int i;

	printk("======== Clear PMFS kernel timing stats ========\n");
	for (i = 0; i < TIMING_NUM; i++) {
		Countstats_pmfs[i] = 0;
		Timingstats_pmfs[i] = 0;
	}
}
