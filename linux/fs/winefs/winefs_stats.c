#include "winefs.h"
#include "inode.h"

const char *winefs_Timingstring[TIMING_NUM] =
{
	"create",
	"new_inode",
	"add_nondir",
	"create_new_trans",
	"create_commit_trans",
	"unlink",
	"remove_entry",
	"unlink_new_trans",
	"unlink_commit_trans",
	"truncate_add",
	"evict_inode",
	"free_tree",
	"free_inode",
	"readdir",
	"xip_read",
	"read_find_blocks",
	"read__winefs_get_block",
	"read_winefs_find_data_blocks",
	"__winefs_find_data_blocks",
	"read_get_inode",
	"xip_write",
	"xip_write_fast",
	"allocate_blocks",
	"internal_write",
	"write_new_trans",
	"write_commit_trans",
	"write_find_blocks",
	"memcpy_read",
	"memcpy_write",
	"alloc_blocks",
	"new_trans",
	"add_logentry",
	"commit_trans",
	"mmap_fault",
	"fsync",
	"recovery",
};

unsigned long long winefs_Timingstats[TIMING_NUM];
u64 winefs_Countstats[TIMING_NUM];

atomic64_t winefs_fsync_pages = ATOMIC_INIT(0);

void winefs_print_IO_stats(void)
{
	printk("=========== WINEFS I/O stats ===========\n");
	printk("Fsync %ld pages\n", atomic64_read(&winefs_fsync_pages));
}

void winefs_print_available_hugepages(struct super_block *sb)
{
	struct winefs_sb_info *sbi = WINEFS_SB(sb);
	int i;
	unsigned long num_hugepages = 0;
	unsigned long num_free_blocks = 0;
	struct free_list *free_list;


	printk("======== WINEFS Available Free Hugepages =======\n");
	for (i = 0; i < sbi->cpus; i++) {
		free_list = winefs_get_free_list(sb, i);
		num_hugepages += free_list->num_blocknode_huge_aligned;
		printk("free list idx %d, free hugepages %lu, free unaligned pages %lu\n",
		       free_list->index, free_list->num_blocknode_huge_aligned,
		       free_list->num_blocknode_unaligned);
		num_free_blocks += free_list->num_free_blocks;
	}
	printk("Total free hugepages %lu, Total free blocks = %lu, Possible free hugepages = %lu\n",
	       num_hugepages, num_free_blocks, num_free_blocks / 512);
}

void winefs_print_timing_stats(void)
{
	int i;

	printk("======== WINEFS kernel timing stats ========\n");
	for (i = 0; i < TIMING_NUM; i++) {
		if (measure_timing_winefs || winefs_Timingstats[i]) {
			printk("%s: count %llu, timing %llu, average %llu\n",
				winefs_Timingstring[i],
				winefs_Countstats[i],
				winefs_Timingstats[i],
				winefs_Countstats[i] ?
				winefs_Timingstats[i] / winefs_Countstats[i] : 0);
		} else {
			printk("%s: count %llu\n",
				winefs_Timingstring[i],
				winefs_Countstats[i]);
		}
	}

	winefs_print_IO_stats();
}

void winefs_clear_stats(void)
{
	int i;

	printk("======== Clear WINEFS kernel timing stats ========\n");
	for (i = 0; i < TIMING_NUM; i++) {
		winefs_Countstats[i] = 0;
		winefs_Timingstats[i] = 0;
	}
}
