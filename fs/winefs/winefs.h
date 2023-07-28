/*
 * BRIEF DESCRIPTION
 *
 * Definitions for the WINEFS filesystem.
 *
 * Copyright 2012-2013 Intel Corporation
 * Copyright 2009-2011 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. , Steve Longerbeam
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __WINEFS_H
#define __WINEFS_H

#include <linux/crc16.h>
#include <linux/mutex.h>
#include <linux/pagemap.h>
#include <linux/rcupdate.h>
#include <linux/types.h>
#include <linux/uio.h>
#include <linux/version.h>
#include <linux/pfn_t.h>
#include <linux/iomap.h>
#include <linux/dax.h>
#include <linux/kthread.h>

#include "winefs_def.h"
#include "journal.h"

#define PAGE_SHIFT_2M 21
#define PAGE_SHIFT_1G 30

#define WINEFS_ASSERT(x)                                                 \
	if (!(x)) {                                                     \
		printk(KERN_WARNING "assertion failed %s:%d: %s\n",     \
	               __FILE__, __LINE__, #x);                         \
	}

/*
 * Debug code
 */
#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#endif

/* #define winefs_dbg(s, args...)         pr_debug(s, ## args) */
#define winefs_dbg(s, args ...)           pr_info(s, ## args)
#define winefs_dbg1(s, args ...)
#define winefs_err(sb, s, args ...)       winefs_error_mng(sb, s, ## args)
#define winefs_warn(s, args ...)          pr_warn(s, ## args)
#define winefs_info(s, args ...)          pr_info(s, ## args)

extern unsigned int winefs_dbgmask;
#define WINEFS_DBGMASK_MMAPHUGE          (0x00000001)
#define WINEFS_DBGMASK_MMAP4K            (0x00000002)
#define WINEFS_DBGMASK_MMAPVERBOSE       (0x00000004)
#define WINEFS_DBGMASK_MMAPVVERBOSE      (0x00000008)
#define WINEFS_DBGMASK_VERBOSE           (0x00000010)
#define WINEFS_DBGMASK_TRANSACTION       (0x00000020)

#define winefs_dbg_mmaphuge(s, args ...)		 \
	((winefs_dbgmask & WINEFS_DBGMASK_MMAPHUGE) ? winefs_dbg(s, args) : 0)
#define winefs_dbg_mmap4k(s, args ...)		 \
	((winefs_dbgmask & WINEFS_DBGMASK_MMAP4K) ? winefs_dbg(s, args) : 0)
#define winefs_dbg_mmapv(s, args ...)		 \
	((winefs_dbgmask & WINEFS_DBGMASK_MMAPVERBOSE) ? winefs_dbg(s, args) : 0)
#define winefs_dbg_mmapvv(s, args ...)		 \
	((winefs_dbgmask & WINEFS_DBGMASK_MMAPVVERBOSE) ? winefs_dbg(s, args) : 0)

#define winefs_dbg_verbose(s, args ...)		 \
	((winefs_dbgmask & WINEFS_DBGMASK_VERBOSE) ? winefs_dbg(s, ##args) : 0)
#define winefs_dbg_trans(s, args ...)		 \
	((winefs_dbgmask & WINEFS_DBGMASK_TRANSACTION) ? winefs_dbg(s, ##args) : 0)

#define winefs_set_bit                   __test_and_set_bit_le
#define winefs_clear_bit                 __test_and_clear_bit_le
#define winefs_find_next_zero_bit                find_next_zero_bit_le

#define clear_opt(o, opt)       (o &= ~WINEFS_MOUNT_ ## opt)
#define set_opt(o, opt)         (o |= WINEFS_MOUNT_ ## opt)
#define test_opt(sb, opt)       (WINEFS_SB(sb)->s_mount_opt & WINEFS_MOUNT_ ## opt)

#define WINEFS_LARGE_INODE_TABLE_SIZE    (0x200000)
/* WINEFS size threshold for using 2M blocks for inode table */
#define WINEFS_LARGE_INODE_TABLE_THREASHOLD    (0x20000000)
/*
 * pmfs inode flags
 *
 * WINEFS_EOFBLOCKS_FL	There are blocks allocated beyond eof
 */
#define WINEFS_EOFBLOCKS_FL      0x20000000
/* Flags that should be inherited by new inodes from their parent. */
#define WINEFS_FL_INHERITED (FS_SECRM_FL | FS_UNRM_FL | FS_COMPR_FL | \
			    FS_SYNC_FL | FS_NODUMP_FL | FS_NOATIME_FL |	\
			    FS_COMPRBLK_FL | FS_NOCOMP_FL | FS_JOURNAL_DATA_FL | \
			    FS_NOTAIL_FL | FS_DIRSYNC_FL)
/* Flags that are appropriate for regular files (all but dir-specific ones). */
#define WINEFS_REG_FLMASK (~(FS_DIRSYNC_FL | FS_TOPDIR_FL))
/* Flags that are appropriate for non-directories/regular files. */
#define WINEFS_OTHER_FLMASK (FS_NODUMP_FL | FS_NOATIME_FL)
#define WINEFS_FL_USER_VISIBLE (FS_FL_USER_VISIBLE | WINEFS_EOFBLOCKS_FL)

#define INODES_PER_BLOCK(bt) (1 << (winefs_blk_type_to_shift[bt] - WINEFS_INODE_BITS))

extern unsigned int winefs_blk_type_to_shift[WINEFS_BLOCK_TYPE_MAX];
extern unsigned int winefs_blk_type_to_size[WINEFS_BLOCK_TYPE_MAX];

/* ======================= Timing ========================= */
enum timing_category {
	create_t,
	new_inode_t,
	add_nondir_t,
	create_new_trans_t,
	create_commit_trans_t,
	unlink_t,
	remove_entry_t,
	unlink_new_trans_t,
	unlink_commit_trans_t,
	truncate_add_t,
	evict_inode_t,
	free_tree_t,
	free_inode_t,
	readdir_t,
	xip_read_t,
	read_find_blocks_t,
	read__winefs_get_block_t,
	read_winefs_find_data_blocks_t,
	__winefs_find_data_blocks_t,
	read_get_inode_t,
	xip_write_t,
	xip_write_fast_t,
	allocate_blocks_t,
	internal_write_t,
	write_new_trans_t,
	write_commit_trans_t,
	write_find_block_t,
	memcpy_r_t,
	memcpy_w_t,
	alloc_blocks_t,
	new_trans_t,
	add_log_t,
	commit_trans_t,
	mmap_fault_t,
	fsync_t,
	recovery_t,
	TIMING_NUM,
};

extern const char *winefs_Timingstring[TIMING_NUM];
extern unsigned long long winefs_Timingstats[TIMING_NUM];
extern u64 winefs_Countstats[TIMING_NUM];

extern int measure_timing_winefs;
extern int winefs_support_clwb;

extern atomic64_t winefs_fsync_pages;

typedef struct timespec64 timing_t;

#define WINEFS_START_TIMING(name, start) \
	{if (measure_timing_winefs) ktime_get_ts64(&start);}

#define WINEFS_END_TIMING(name, start) \
	{if (measure_timing_winefs) { \
		timing_t end; \
		ktime_get_ts64(&end); \
		winefs_Timingstats[name] += \
			(end.tv_sec - start.tv_sec) * 1000000000 + \
			(end.tv_nsec - start.tv_nsec); \
	} \
	winefs_Countstats[name]++; \
	}


/* Function Prototypes */
extern void winefs_error_mng(struct super_block *sb, const char *fmt, ...);

/* file.c */
extern int winefs_mmap(struct file *file, struct vm_area_struct *vma);

/* balloc.c */
struct winefs_range_node *winefs_alloc_range_node_atomic(struct super_block *sb);
extern struct winefs_range_node *winefs_alloc_blocknode(struct super_block *sb);
extern void winefs_free_blocknode(struct super_block *sb, struct winefs_range_node *node);
extern void winefs_init_blockmap(struct super_block *sb,
			       unsigned long init_used_size, int recovery);
extern int winefs_free_blocks(struct super_block *sb, unsigned long blocknr, int num,
	unsigned short btype);
extern int winefs_new_blocks(struct super_block *sb, unsigned long *blocknr,
			   unsigned int num, unsigned short btype, int zero,
			   int cpu);
extern unsigned long winefs_count_free_blocks(struct super_block *sb);
extern unsigned int winefs_get_free_numa_node(struct super_block *sb);

/* dir.c */
extern int winefs_add_entry(winefs_transaction_t *trans,
		struct dentry *dentry, struct inode *inode);
extern int winefs_remove_entry(winefs_transaction_t *trans,
		struct dentry *dentry, struct inode *inode);

/* namei.c */
extern struct dentry *winefs_get_parent(struct dentry *child);

/* inode.c */

/* ioctl.c */
extern long winefs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
extern long winefs_compat_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg);
#endif

/* super.c */
#ifdef CONFIG_WINEFS_TEST
extern struct winefs_super_block *get_winefs_super(void);
#endif
extern void __winefs_free_blocknode(struct winefs_range_node *bnode);
extern struct super_block *winefs_read_super(struct super_block *sb, void *data,
	int silent);
extern int winefs_statfs(struct dentry *d, struct kstatfs *buf);
extern int winefs_remount(struct super_block *sb, int *flags, char *data);

/* symlink.c */
extern int winefs_block_symlink(struct inode *inode, const char *symname,
	int len);

/* Inline functions start here */

/* Mask out flags that are inappropriate for the given type of inode. */
static inline __le32 winefs_mask_flags(umode_t mode, __le32 flags)
{
	flags &= cpu_to_le32(WINEFS_FL_INHERITED);
	if (S_ISDIR(mode))
		return flags;
	else if (S_ISREG(mode))
		return flags & cpu_to_le32(WINEFS_REG_FLMASK);
	else
		return flags & cpu_to_le32(WINEFS_OTHER_FLMASK);
}

static inline int winefs_calc_checksum(u8 *data, int n)
{
	u16 crc = 0;

	crc = crc16(~0, (__u8 *)data + sizeof(__le16), n - sizeof(__le16));
	if (*((__le16 *)data) == cpu_to_le16(crc))
		return 0;
	else
		return 1;
}

struct winefs_blocknode_lowhigh {
       __le64 block_low;
       __le64 block_high;
};

enum bm_type {
	BM_4K = 0,
	BM_2M,
	BM_1G,
};

struct single_scan_bm {
	unsigned long bitmap_size;
	unsigned long *bitmap;
};

struct scan_bitmap {
	struct single_scan_bm scan_bm_4K;
	struct single_scan_bm scan_bm_2M;
	struct single_scan_bm scan_bm_1G;
};

struct inode_map {
	struct mutex inode_table_mutex;
	struct rb_root inode_inuse_tree;
	unsigned long num_range_node_inode;
	struct winefs_range_node *first_inode_range;
	int allocated;
	int freed;
};

/*
 * WINEFS super-block data in memory
 */
struct winefs_sb_info {
	/*
	 * base physical and virtual address of WINEFS (which is also
	 * the pointer to the super block)
	 */
	struct block_device *s_bdev;
	struct dax_device *s_dax_dev;
	phys_addr_t	phys_addr;
	phys_addr_t     phys_addr_2;
	void		*virt_addr;
	void            *virt_addr_2;
	struct list_head block_inuse_head;
	unsigned long	*block_start;
	unsigned long	*block_end;
	unsigned long	num_free_blocks;
	struct mutex 	s_lock;	/* protects the SB's buffer-head */
	unsigned long   num_blocks;
	struct free_list *hole_free_list;

	int cpus;

	/*
	 * Backing store option:
	 * 1 = no load, 2 = no store,
	 * else do both
	 */
	unsigned int	winefs_backing_option;

	/* Mount options */
	unsigned long	bpi;
	unsigned long	num_inodes;
	unsigned long	blocksize;
	unsigned long	initsize;
	unsigned long   initsize_2;
	unsigned long   pmem_size;
	unsigned long   pmem_size_2;
	unsigned long	s_mount_opt;
	kuid_t		uid;    /* Mount uid for root directory */
	kgid_t		gid;    /* Mount gid for root directory */
	umode_t		mode;   /* Mount mode for root directory */
	atomic_t	next_generation;
	/* inode tracking */
	struct mutex inode_table_mutex;
	unsigned int	s_inodes_count;  /* total inodes count (used or free) */
	unsigned int	s_free_inodes_count;    /* free inodes count */
	unsigned int	s_inodes_used_count;
	unsigned int	s_free_inode_hint;

	unsigned long num_blocknode_allocated;
	unsigned long num_inodenode_allocated;

	unsigned long head_reserved_blocks;

	/* Journaling related structures */
	atomic64_t    next_transaction_id;
	uint32_t    jsize;
	void       **journal_base_addr;
	struct mutex *journal_mutex;
	struct task_struct *log_cleaner_thread;
	wait_queue_head_t  log_cleaner_wait;
	bool redo_log;

	/* truncate list related structures */
	struct list_head s_truncate;
	struct mutex s_truncate_lock;

	struct inode_map *inode_maps;

	/* Decide new inode map id */
	unsigned long map_id;

	/* Number of NUMA nodes */
	int num_numa_nodes;

	/* process -> NUMA node mapping */
	int num_parallel_procs;
	struct process_numa *process_numa;

	/* Struct to hold NUMA node for each CPU */
	u8 *cpu_numa_node;

	/* struct to hold cpus for each NUMA node */
	struct numa_node_cpus *numa_cpus;

	/* Per-CPU free blocks list */
	struct free_list *free_lists;
	unsigned long per_list_blocks;
};

struct process_numa {
	int tgid;
	int numa_node;
};

struct numa_node_cpus {
	int *cpus;
	int num_cpus;
	struct cpumask cpumask;
};

struct winefs_range_node_lowhigh {
	__le64 range_low;
	__le64 range_high;
};

#define	RANGENODE_PER_PAGE	256

struct winefs_range_node {
	struct rb_node node;
	struct vm_area_struct *vma;
	unsigned long mmap_entry;
	union {
		/* Block, inode */
		struct {
			unsigned long range_low;
			unsigned long range_high;
		};
		/* Dir node */
		struct {
			unsigned long hash;
			void *direntry;
		};
	};
};

struct vma_item {
	struct rb_node node;
	struct vm_area_struct *vma;
	unsigned long mmap_entry;
};

static inline struct winefs_sb_info *WINEFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

/* If this is part of a read-modify-write of the super block,
 * winefs_memunlock_super() before calling! */
static inline struct winefs_super_block *winefs_get_super(struct super_block *sb)
{
	struct winefs_sb_info *sbi = WINEFS_SB(sb);

	return (struct winefs_super_block *)sbi->virt_addr;
}

static inline winefs_journal_t *winefs_get_journal(struct super_block *sb, int cpu)
{
	struct winefs_super_block *ps = winefs_get_super(sb);

	return (winefs_journal_t *)((char *)ps +
				   (le64_to_cpu(ps->s_journal_offset)) +
				   (cpu * CACHELINE_SIZE));
}

static inline struct winefs_inode *winefs_get_inode_table(struct super_block *sb)
{
	struct winefs_super_block *ps = winefs_get_super(sb);

	return (struct winefs_inode *)((char *)ps +
			le64_to_cpu(ps->s_inode_table_offset));
}

static inline struct winefs_super_block *winefs_get_redund_super(struct super_block *sb)
{
	struct winefs_sb_info *sbi = WINEFS_SB(sb);

	return (struct winefs_super_block *)(sbi->virt_addr + WINEFS_SB_SIZE);
}

/* If this is part of a read-modify-write of the block,
 * winefs_memunlock_block() before calling! */
static inline void *winefs_get_block(struct super_block *sb, u64 block)
{
	struct winefs_super_block *ps = winefs_get_super(sb);

	return block ? ((void *)ps + block) : NULL;
}

static inline int winefs_get_reference(struct super_block *sb, u64 block,
	void *dram, void **nvmm, size_t size)
{
	int rc = 0;

	*nvmm = winefs_get_block(sb, block);
	/* rc = memcpy_mcsafe(dram, *nvmm, size); */
	if (memcpy(dram, *nvmm, size) == NULL)
		rc = -1;
	return rc;
}

static inline int winefs_get_numa_node(struct super_block *sb, int cpuid)
{
	struct winefs_sb_info *sbi = WINEFS_SB(sb);

	return sbi->cpu_numa_node[cpuid];
}

/* uses CPU instructions to atomically write up to 8 bytes */
static inline void winefs_memcpy_atomic (void *dst, const void *src, u8 size)
{
	switch (size) {
		case 1: {
			volatile u8 *daddr = dst;
			const u8 *saddr = src;
			*daddr = *saddr;
			break;
		}
		case 2: {
			volatile __le16 *daddr = dst;
			const u16 *saddr = src;
			*daddr = cpu_to_le16(*saddr);
			break;
		}
		case 4: {
			volatile __le32 *daddr = dst;
			const u32 *saddr = src;
			*daddr = cpu_to_le32(*saddr);
			break;
		}
		case 8: {
			volatile __le64 *daddr = dst;
			const u64 *saddr = src;
			*daddr = cpu_to_le64(*saddr);
			break;
		}
		default:
			winefs_dbg("error: memcpy_atomic called with %d bytes\n", size);
			//BUG();
	}
}

/* assumes the length to be 4-byte aligned */
static inline void memset_nt(void *dest, uint32_t dword, size_t length)
{
	uint64_t dummy1, dummy2;
	uint64_t qword = ((uint64_t)dword << 32) | dword;

	asm volatile ("movl %%edx,%%ecx\n"
		"andl $63,%%edx\n"
		"shrl $6,%%ecx\n"
		"jz 9f\n"
		"1:      movnti %%rax,(%%rdi)\n"
		"2:      movnti %%rax,1*8(%%rdi)\n"
		"3:      movnti %%rax,2*8(%%rdi)\n"
		"4:      movnti %%rax,3*8(%%rdi)\n"
		"5:      movnti %%rax,4*8(%%rdi)\n"
		"8:      movnti %%rax,5*8(%%rdi)\n"
		"7:      movnti %%rax,6*8(%%rdi)\n"
		"8:      movnti %%rax,7*8(%%rdi)\n"
		"leaq 64(%%rdi),%%rdi\n"
		"decl %%ecx\n"
		"jnz 1b\n"
		"9:     movl %%edx,%%ecx\n"
		"andl $7,%%edx\n"
		"shrl $3,%%ecx\n"
		"jz 11f\n"
		"10:     movnti %%rax,(%%rdi)\n"
		"leaq 8(%%rdi),%%rdi\n"
		"decl %%ecx\n"
		"jnz 10b\n"
		"11:     movl %%edx,%%ecx\n"
		"shrl $2,%%ecx\n"
		"jz 12f\n"
		"movnti %%eax,(%%rdi)\n"
		"12:\n"
		: "=D"(dummy1), "=d" (dummy2) : "D" (dest), "a" (qword), "d" (length) : "memory", "rcx");
}

static inline unsigned long BKDRHash(const char *str, int length)
{
	unsigned int seed = 131;
	unsigned long hash = 0;
	int i;

	for (i = 0; i < length; i++)
		hash = hash * seed + (*str++);

	return hash;
}

static inline u64
winefs_get_block_off(struct super_block *sb, unsigned long blocknr,
		    unsigned short btype)
{
	return (u64)blocknr << PAGE_SHIFT;
}

static inline int winefs_get_cpuid(struct super_block *sb)
{
	struct winefs_sb_info *sbi = WINEFS_SB(sb);

	return smp_processor_id() % sbi->cpus;
}

static inline int get_block_cpuid(struct winefs_sb_info *sbi,
	unsigned long blocknr)
{
	unsigned long temp_blocknr = 0;
	int cpuid = blocknr / sbi->per_list_blocks;

	if (sbi->num_numa_nodes == 2) {
		if (sbi->cpus == 96 || sbi->cpus == 32) {
			if (blocknr >= sbi->block_start[1]) {
				temp_blocknr = blocknr - (sbi->block_start[1] - sbi->block_end[0]);
				cpuid = temp_blocknr / sbi->per_list_blocks;
			}
		}
	}

	if (sbi->num_numa_nodes == 2) {
		if (sbi->cpus == 96) {
			if (cpuid >= 24 && cpuid < 48) {
				cpuid += 24;
			} else if (cpuid >= 48 && cpuid < 72) {
				cpuid -= 24;
			}
		}
		else if (sbi->cpus == 32) {
			if (cpuid >= 8 && cpuid < 16) {
				cpuid += 8;
			} else if (cpuid >= 16 && cpuid < 24) {
				cpuid -= 8;
			}
		}
	}
	return cpuid;
}

static inline unsigned long
winefs_get_numblocks(unsigned short btype)
{
	unsigned long num_blocks;

	if (btype == WINEFS_BLOCK_TYPE_4K) {
		num_blocks = 1;
	} else if (btype == WINEFS_BLOCK_TYPE_2M) {
		num_blocks = 512;
	} else {
		//btype == WINEFS_BLOCK_TYPE_1G 
		num_blocks = 0x40000;
	}
	return num_blocks;
}

static inline unsigned long
winefs_get_blocknr(struct super_block *sb, u64 block, unsigned short btype)
{
	return block >> PAGE_SHIFT;
}

static inline unsigned long winefs_get_pfn(struct super_block *sb, u64 block)
{
	return (WINEFS_SB(sb)->phys_addr + block) >> PAGE_SHIFT;
}

static inline int winefs_is_mounting(struct super_block *sb)
{
	struct winefs_sb_info *sbi = (struct winefs_sb_info *)sb->s_fs_info;
	return sbi->s_mount_opt & WINEFS_MOUNT_MOUNTING;
}

static inline int is_dir_init_entry(struct super_block *sb,
	struct winefs_direntry *entry)
{
	if (entry->name_len == 1 && strncmp(entry->name, ".", 1) == 0)
		return 1;
	if (entry->name_len == 2 && strncmp(entry->name, "..", 2) == 0)
		return 1;

	return 0;
}

#include "wprotect.h"
#include "balloc.h"

/*
 * Inodes and files operations
 */

/* dir.c */
extern const struct file_operations winefs_dir_operations;
int winefs_insert_dir_tree(struct super_block *sb,
			 struct winefs_inode_info_header *sih, const char *name,
			 int namelen, struct winefs_direntry *direntry);
int winefs_remove_dir_tree(struct super_block *sb,
			 struct winefs_inode_info_header *sih, const char *name, int namelen,
			 struct winefs_direntry **create_dentry);
void winefs_delete_dir_tree(struct super_block *sb,
			  struct winefs_inode_info_header *sih);
struct winefs_direntry *winefs_find_dentry(struct super_block *sb,
				       struct winefs_inode *pi, struct inode *inode,
				       const char *name, unsigned long name_len);

/* xip.c */
int winefs_iomap_begin(struct inode *inode, loff_t offset, loff_t length,
		     unsigned int flags, struct iomap *iomap, bool taking_lock);
int winefs_iomap_end(struct inode *inode, loff_t offset, loff_t length,
		   ssize_t written, unsigned int flags, struct iomap *iomap);


/* file.c */
extern const struct inode_operations winefs_file_inode_operations;
extern const struct file_operations winefs_xip_file_operations;
int winefs_fsync(struct file *file, loff_t start, loff_t end, int datasync);

/* inode.c */
extern const struct address_space_operations winefs_aops_xip;

/* bbuild.c */
void winefs_init_header(struct super_block *sb,
		      struct winefs_inode_info_header *sih, u16 i_mode);
void winefs_save_blocknode_mappings(struct super_block *sb);
void winefs_save_inode_list(struct super_block *sb);
int winefs_recovery(struct super_block *sb, unsigned long size, unsigned long size_2);

/* namei.c */
extern const struct inode_operations winefs_dir_inode_operations;
extern const struct inode_operations winefs_special_inode_operations;

/* symlink.c */
extern const struct inode_operations winefs_symlink_inode_operations;

int winefs_check_integrity(struct super_block *sb,
	struct winefs_super_block *super);
void *winefs_ioremap(struct super_block *sb, phys_addr_t phys_addr,
	ssize_t size);

int winefs_check_dir_entry(const char *function, struct inode *dir,
			  struct winefs_direntry *de, u8 *base,
			  unsigned long offset);

static inline int winefs_match(int len, const char *const name,
			      struct winefs_direntry *de)
{
	if (len == de->name_len && de->ino && !memcmp(de->name, name, len))
		return 1;
	return 0;
}

int winefs_search_dirblock(u8 *blk_base, struct inode *dir, struct qstr *child,
			  unsigned long offset,
			  struct winefs_direntry **res_dir,
			  struct winefs_direntry **prev_dir);

#define ANY_CPU                (65536)

/* winefs_stats.c */
#define	WINEFS_PRINT_TIMING	0xBCD00010
#define	WINEFS_CLEAR_STATS	0xBCD00011
#define WINEFS_GET_AVAILABLE_HUGEPAGES 0xBCD00012

void winefs_print_timing_stats(void);
void winefs_print_available_hugepages(struct super_block *sb);
void winefs_clear_stats(void);

#endif /* __WINEFS_H */
