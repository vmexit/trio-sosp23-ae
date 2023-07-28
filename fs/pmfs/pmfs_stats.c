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

unsigned long Timingstats_pmfs[TIMING_NUM];
DEFINE_PER_CPU(unsigned long[TIMING_NUM], Timingstats_percpu_pmfs);
unsigned long Countstats_pmfs[TIMING_NUM];
DEFINE_PER_CPU(unsigned long[TIMING_NUM], Countstats_percpu_pmfs);

atomic64_t fsync_pages = ATOMIC_INIT(0);

void pmfs_print_IO_stats(void)
{
	printk("=========== PMFS I/O stats ===========\n");
	printk("Fsync %lld pages\n", atomic64_read(&fsync_pages));
}

static void pmfs_get_timing_stats(void) {
  int i;
  int cpu;

  for (i = 0; i < TIMING_NUM; i++) {
    Timingstats_pmfs[i] = 0;
    Countstats_pmfs[i] = 0;
    for_each_possible_cpu(cpu) {
      Timingstats_pmfs[i] += per_cpu(Timingstats_percpu_pmfs[i], cpu);
      Countstats_pmfs[i] += per_cpu(Countstats_percpu_pmfs[i], cpu);
    }
  }
}

void pmfs_print_timing_stats(void) {
  int i;

  pmfs_get_timing_stats();

  printk("======== PMFS kernel timing stats ========\n");
  for (i = 0; i < TIMING_NUM; i++) {
    if (measure_timing_pmfs || Timingstats_pmfs[i]) {
      printk("%s: count %lu, timing %lu, average %lu\n", Timingstring_pmfs[i],
             Countstats_pmfs[i], Timingstats_pmfs[i],
             Countstats_pmfs[i] ? Timingstats_pmfs[i] / Countstats_pmfs[i] : 0);
    } else {
      printk("%s: count %lu\n", Timingstring_pmfs[i], Countstats_pmfs[i]);
    }
  }

  pmfs_print_IO_stats();
}

void pmfs_clear_stats(void) {
  int i;
  int cpu;

  printk("======== Clear PMFS kernel timing stats ========\n");
  for (i = 0; i < TIMING_NUM; i++) {
    Countstats_pmfs[i] = 0;
    Timingstats_pmfs[i] = 0;

    for_each_possible_cpu(cpu) {
      per_cpu(Timingstats_percpu_pmfs[i], cpu) = 0;
      per_cpu(Countstats_percpu_pmfs[i], cpu) = 0;
    }
  }
}
