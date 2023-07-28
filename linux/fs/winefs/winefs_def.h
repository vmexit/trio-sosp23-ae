/*
 * FILE NAME include/linux/winefs_fs.h
 *
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
#ifndef _LINUX_WINEFS_DEF_H
#define _LINUX_WINEFS_DEF_H

#include <linux/types.h>
#include <linux/magic.h>

#define	WINEFS_SUPER_MAGIC	0xEFFC

/*
 * The WINEFS filesystem constants/structures
 */

/*
 * Mount flags
 */
#define WINEFS_MOUNT_PROTECT 0x000001            /* wprotect CR0.WP */
#define WINEFS_MOUNT_XATTR_USER 0x000002         /* Extended user attributes */
#define WINEFS_MOUNT_POSIX_ACL 0x000004          /* POSIX Access Control Lists */
#define WINEFS_MOUNT_XIP 0x000008                /* Execute in place */
#define WINEFS_MOUNT_ERRORS_CONT 0x000010        /* Continue on errors */
#define WINEFS_MOUNT_ERRORS_RO 0x000020          /* Remount fs ro on errors */
#define WINEFS_MOUNT_ERRORS_PANIC 0x000040       /* Panic on errors */
#define WINEFS_MOUNT_HUGEMMAP 0x000080           /* Huge mappings with mmap */
#define WINEFS_MOUNT_HUGEIOREMAP 0x000100        /* Huge mappings with ioremap */
#define WINEFS_MOUNT_PROTECT_OLD 0x000200        /* wprotect PAGE RW Bit */
#define WINEFS_MOUNT_FORMAT      0x000400        /* was FS formatted on mount? */
#define WINEFS_MOUNT_MOUNTING    0x000800        /* FS currently being mounted */
#define WINEFS_MOUNT_STRICT      0x002000       /* atomic data guarantees */

/*
 * Maximal count of links to a file
 */
#define WINEFS_LINK_MAX          32000

#define WINEFS_DEF_BLOCK_SIZE_4K 4096

#define WINEFS_INODE_SIZE 128    /* must be power of two */
#define WINEFS_INODE_BITS   7
#define INODE_TABLE0_START 4
#define INODE_TABLE_NUM_BLOCKS 5 /* Can host inode tables for up to 64 x 5 cpus */
#define HEAD_RESERVED_BLOCKS 64

#define WINEFS_NAME_LEN 255

#define MAX_CPUS 1024

/*
 * Structure of a directory entry in WINEFS.
 */
struct winefs_direntry {
	__le64	ino;                    /* inode no pointed to by this entry */
	__le16	de_len;                 /* length of this directory entry */
	u8	name_len;               /* length of the directory entry name */
	u8	file_type;              /* file type */
	char	name[WINEFS_NAME_LEN];   /* File name */
};

#define WINEFS_DIR_PAD            4
#define WINEFS_DIR_ROUND          (WINEFS_DIR_PAD - 1)
#define WINEFS_DIR_REC_LEN(name_len)  (((name_len) + 12 + WINEFS_DIR_ROUND) & \
				      ~WINEFS_DIR_ROUND)

/* WINEFS supported data blocks */
#define WINEFS_BLOCK_TYPE_4K     0
#define WINEFS_BLOCK_TYPE_2M     1
#define WINEFS_BLOCK_TYPE_1G     2
#define WINEFS_BLOCK_TYPE_MAX    3

#define META_BLK_SHIFT 9

/*
 * Play with this knob to change the default block type.
 * By changing the WINEFS_DEFAULT_BLOCK_TYPE to 2M or 1G,
 * we should get pretty good coverage in testing.
 */
#define WINEFS_DEFAULT_BLOCK_TYPE WINEFS_BLOCK_TYPE_4K

/*
 * #define WINEFS_NAME_LEN (WINEFS_INODE_SIZE - offsetof(struct winefs_inode,
 *         i_d.d_name) - 1)
 */

/* #define WINEFS_SB_SIZE 128 */ /* must be power of two */
#define WINEFS_SB_SIZE 8192       /* must be power of two */

typedef struct winefs_journal {
	__le64     base;
	__le32     size;
	__le32     head;
	/* the next three fields must be in the same order and together.
	 * tail and gen_id must fall in the same 8-byte quadword */
	__le32     tail;
	__le16     gen_id;   /* generation id of the log */
	__le16     pad;
	__le16     redo_logging;
} winefs_journal_t;


/*
 * Structure of the super block in WINEFS
 * The fields are partitioned into static and dynamic fields. The static fields
 * never change after file system creation. This was primarily done because
 * winefs_get_block() returns NULL if the block offset is 0 (helps in catching
 * bugs). So if we modify any field using journaling (for consistency), we 
 * will have to modify s_sum which is at offset 0. So journaling code fails.
 * This (static+dynamic fields) is a temporary solution and can be avoided
 * once the file system becomes stable and winefs_get_block() returns correct
 * pointers even for offset 0.
 */
struct winefs_super_block {
	/* static fields. they never change after file system creation.
	 * checksum only validates up to s_start_dynamic field below */
	__le16		s_sum;              /* checksum of this sb */
	__le16		s_magic;            /* magic signature */
	__le32		s_blocksize;        /* blocksize in bytes */
	__le64		s_size;             /* total size of fs in bytes */
	__le64          s_size_1;
	__le64          s_size_2;
	char		s_volume_name[16];  /* volume name */
	/* points to the location of winefs_journal_t */
	__le64          s_journal_offset;
	/* points to the location of struct winefs_inode for the inode table */
	__le64          s_inode_table_offset;

	__le64       s_start_dynamic;

	/* all the dynamic fields should go here */
	/* s_mtime and s_wtime should be together and their order should not be
	 * changed. we use an 8 byte write to update both of them atomically */
	__le32		s_mtime;            /* mount time */
	__le32		s_wtime;            /* write time */
	/* fields for fast mount support. Always keep them together */
	__le64		s_num_blocknode_allocated;
	__le64          s_num_inodenode_allocated;
	__le64		s_num_free_blocks;
	__le32		s_inodes_count;
	__le32		s_free_inodes_count;
	__le32		s_inodes_used_count;
	__le32		s_free_inode_hint;
};

#define WINEFS_SB_STATIC_SIZE(ps) ((u64)&ps->s_start_dynamic - (u64)ps)

/* the above fast mount fields take total 32 bytes in the super block */
#define WINEFS_FAST_MOUNT_FIELD_SIZE  (36)

/* The root inode follows immediately after the redundant super block */
#define WINEFS_ROOT_INO (1)
#define WINEFS_BLOCKNODE_IN0 (2)
#define WINEFS_INODELIST_IN0 (3)

#define WINEFS_FREE_INODE_HINT_START      (4)
#define WINEFS_NORMAL_INODE_START         (32)

/* ======================= Write ordering ========================= */

#define CACHELINE_SIZE  (64)
#define CACHELINE_MASK  (~(CACHELINE_SIZE - 1))
#define CACHELINE_ALIGN(addr) (((addr)+CACHELINE_SIZE-1) & CACHELINE_MASK)

#define X86_FEATURE_PCOMMIT	( 9*32+22) /* PCOMMIT instruction */
#define X86_FEATURE_CLFLUSHOPT	( 9*32+23) /* CLFLUSHOPT instruction */
#define X86_FEATURE_CLWB	( 9*32+24) /* CLWB instruction */

static inline bool arch_has_pcommit(void)
{
	return static_cpu_has(X86_FEATURE_PCOMMIT);
}

static inline bool arch_has_clwb(void)
{
	return static_cpu_has(X86_FEATURE_CLWB);
}

extern int winefs_support_clwb;
extern int winefs_support_pcommit;

#define _mm_clflush(addr)\
	asm volatile("clflush %0" : "+m" (*(volatile char *)(addr)))
#define _mm_clflushopt(addr)\
	asm volatile(".byte 0x66; clflush %0" : "+m" (*(volatile char *)(addr)))
#define _mm_clwb(addr)\
	asm volatile(".byte 0x66; xsaveopt %0" : "+m" (*(volatile char *)(addr)))
#define _mm_pcommit()\
	asm volatile(".byte 0x66, 0x0f, 0xae, 0xf8")

/* Provides ordering from all previous clflush too */
static inline void PERSISTENT_MARK(void)
{
	/* TODO: Fix me. */
}

static inline void PERSISTENT_BARRIER(void)
{
	asm volatile ("sfence\n" : : );
	if (winefs_support_pcommit) {
		/* Do nothing */
	}
}

static inline void winefs_flush_buffer(void *buf, uint32_t len, bool fence)
{
	uint32_t i;
	len = len + ((unsigned long)(buf) & (CACHELINE_SIZE - 1));
	if (winefs_support_clwb) {
		for (i = 0; i < len; i += CACHELINE_SIZE)
			_mm_clwb(buf + i);
	} else {
		for (i = 0; i < len; i += CACHELINE_SIZE)
			_mm_clflush(buf + i);
	}
	/* Do a fence only if asked. We often don't need to do a fence
	 * immediately after clflush because even if we get context switched
	 * between clflush and subsequent fence, the context switch operation
	 * provides implicit fence. */
	if (fence)
		PERSISTENT_BARRIER();
}

#endif /* _LINUX_WINEFS_DEF_H */
