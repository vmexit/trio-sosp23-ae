/*
 * Mostly Copied from NOVA persistent memory management
 *
 * Copyright 2015-2016 Regents of the University of California,
 * UCSD Non-Volatile Systems Lab, Andiry Xu <jix024@cs.ucsd.edu>
 * Copyright 2012-2013 Intel Corporation
 * Copyright 2009-2011 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. , Steve Longerbeam
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <linux/bitops.h>
#include <linux/fs.h>
#include <linux/percpu.h>
#include <linux/random.h>

#include "balloc.h"
#include "delegation.h"
#include "pmfs.h"

DEFINE_PER_CPU(u32, seed);

/* rbtree wrapper, should put them into a seperate directory */

static inline int pmfs_rbtree_compare_range_node(struct pmfs_range_node *curr,
						 unsigned long key, enum node_type type)
{
	if (type == NODE_DIR) {
		if (key < curr->hash)
			return -1;
		if (key > curr->hash)
			return 1;
		return 0;
	}
	/* Block and inode */
	if (key < curr->range_low)
		return -1;
	if (key > curr->range_high)
		return 1;

	return 0;
}

int pmfs_find_range_node(struct rb_root *tree, unsigned long key,
		enum node_type type, struct pmfs_range_node **ret_node)
{
	struct pmfs_range_node *curr = NULL;
	struct rb_node *temp;
	int compVal;
	int ret = 0;

	temp = tree->rb_node;

	while (temp) {
		curr = container_of(temp, struct pmfs_range_node, node);
		compVal = pmfs_rbtree_compare_range_node(curr, key, type);

		if (compVal == -1) {
			temp = temp->rb_left;
		} else if (compVal == 1) {
			temp = temp->rb_right;
		} else {
			ret = 1;
			break;
		}
	}

	*ret_node = curr;
	return ret;
}

int pmfs_insert_range_node(struct rb_root *tree,
				  struct pmfs_range_node *new_node, enum node_type type)
{
	struct pmfs_range_node *curr;
	struct rb_node **temp, *parent;
	int compVal;

	temp = &(tree->rb_node);
	parent = NULL;

	while (*temp) {
		curr = container_of(*temp, struct pmfs_range_node, node);
		compVal = pmfs_rbtree_compare_range_node(curr,
							 new_node->range_low, type);
		parent = *temp;

		if (compVal == -1) {
			temp = &((*temp)->rb_left);
		} else if (compVal == 1) {
			temp = &((*temp)->rb_right);
		} else {
			pmfs_dbg("%s: entry %lu - %lu already exists: "
				 "%lu - %lu\n",
				 __func__, new_node->range_low,
				 new_node->range_high, curr->range_low,
				 curr->range_high);
			return -EINVAL;
		}
	}

	rb_link_node(&new_node->node, parent, temp);
	rb_insert_color(&new_node->node, tree);

	return 0;
}

void pmfs_destroy_range_node_tree(struct rb_root *tree)
{
	struct pmfs_range_node *curr;
	struct rb_node *temp;

	temp = rb_first(tree);
	while (temp) {
		curr = container_of(temp, struct pmfs_range_node, node);
		temp = rb_next(temp);
		rb_erase(&curr->node, tree);
		pmfs_free_range_node(curr);
	}
}

int pmfs_insert_blocktree(struct rb_root *tree,
			  struct pmfs_range_node *new_node)
{
	int ret;

	ret = pmfs_insert_range_node(tree, new_node, NODE_BLOCK);
	if (ret)
		pmfs_dbg("ERROR: %s failed %d\n", __func__, ret);

	return ret;
}

/* init */

int pmfs_alloc_block_free_lists(struct super_block *sb)
{
	struct pmfs_sb_info *sbi = PMFS_SB(sb);
	struct pmfs_free_list *free_list;
	int i, j;

	sbi->free_lists = kcalloc(sbi->cpus * sbi->sockets,
				  sizeof(struct pmfs_free_list), GFP_KERNEL);

	if (!sbi->free_lists)
		return -ENOMEM;

	for (i = 0; i < sbi->cpus; i++)
		for (j = 0; j < sbi->sockets; j++) {
			free_list = pmfs_get_free_list(sb, i, j);
			free_list->block_free_tree = RB_ROOT;
			spin_lock_init(&free_list->s_lock);
		}

	return 0;
}

void pmfs_delete_free_lists(struct super_block *sb)
{
	struct pmfs_sb_info *sbi = PMFS_SB(sb);

	/* Each tree is freed in save_blocknode_mappings */
	kfree(sbi->free_lists);
	sbi->free_lists = NULL;
}

// Initialize a free list.  Each CPU gets an equal share of the block space to
// manage.
static void pmfs_init_free_list(struct super_block *sb,
				struct pmfs_free_list *free_list, int cpu,
				int socket)
{
	struct pmfs_sb_info *sbi = PMFS_SB(sb);
	unsigned long per_list_blocks;

	unsigned long size = sbi->block_info[socket].end_block -
			     sbi->block_info[socket].start_block + 1;

	per_list_blocks = size / sbi->cpus;

	if (cpu != sbi->cpus - 1) {
		free_list->block_start = sbi->block_info[socket].start_block +
					 per_list_blocks * cpu;

		free_list->block_end =
			free_list->block_start + per_list_blocks - 1;
	} else {
		free_list->block_start = sbi->block_info[socket].start_block +
					 per_list_blocks * cpu;

		free_list->block_end = sbi->block_info[socket].end_block;
	}

	if (cpu == 0 && socket == sbi->head_socket)
		free_list->block_start += sbi->head_reserved_blocks;
}

void pmfs_init_blockmap(struct super_block *sb, int recovery)
{
	struct pmfs_sb_info *sbi = PMFS_SB(sb);
	struct rb_root *tree;
	struct pmfs_range_node *blknode;
	struct pmfs_free_list *free_list;
	int i, j;
	int ret;

	/* Divide the block range among per-CPU free lists */
	for (i = 0; i < sbi->cpus; i++)
		for (j = 0; j < sbi->sockets; j++) {
			free_list = pmfs_get_free_list(sb, i, j);
			tree = &(free_list->block_free_tree);
			pmfs_init_free_list(sb, free_list, i, j);

			/* For recovery, update these fields later */
			if (recovery == 0) {
				free_list->num_free_blocks =
					free_list->block_end -
					free_list->block_start + 1;

				blknode = pmfs_alloc_blocknode(sb);

				if (blknode == NULL)
					BUG();

				blknode->range_low = free_list->block_start;
				blknode->range_high = free_list->block_end;

				ret = pmfs_insert_blocktree(tree, blknode);
				if (ret) {
					pmfs_err(sb, "%s failed\n", __func__);
					pmfs_free_blocknode(blknode);
					return;
				}
				free_list->first_node = blknode;
				free_list->last_node = blknode;
				free_list->num_blocknode = 1;
			}

			pmfs_dbg_alloc(
				"%s: free list, addr: %lx, cpu %d, socket %d,  block "
				"start %lu, end %lu, "
				"%lu free blocks\n",
				__func__, (unsigned long)free_list, i, j,
				free_list->block_start, free_list->block_end,
				free_list->num_free_blocks);
		}
}

/* why the heck pass sb not sbi ? */
/* Which cpu or socket this block belongs to */
static void pmfs_block_to_cpu_socket(struct super_block *sb, int blocknr,
				     int *cpu, int *socket)
{
	int cpu_tmp = 0;
	unsigned long size = 0;
	struct pmfs_sb_info *sbi = PMFS_SB(sb);

	*socket = pmfs_block_to_socket(sbi, blocknr);

	size = sbi->block_info[*socket].end_block -
	       sbi->block_info[*socket].start_block + 1;

	cpu_tmp = (blocknr - sbi->block_info[*socket].start_block) /
		  (size / sbi->cpus);

	/* The remainder of the last cpu */
	if (cpu_tmp >= sbi->cpus)
		cpu_tmp = sbi->cpus - 1;

	(*cpu) = cpu_tmp;

	return;
}

/* Used for both block free tree and inode inuse tree */
int pmfs_find_free_slot(struct rb_root *tree, unsigned long range_low,
			unsigned long range_high, struct pmfs_range_node **prev,
			struct pmfs_range_node **next)
{
	struct pmfs_range_node *ret_node = NULL;
	struct rb_node *tmp;
	int ret;

	ret = pmfs_find_range_node(tree, range_low, NODE_BLOCK, &ret_node);
	if (ret) {
		pmfs_dbg("%s ERROR: %lu - %lu already in free list\n", __func__,
			 range_low, range_high);
		return -EINVAL;
	}

	if (!ret_node) {
		*prev = *next = NULL;
	} else if (ret_node->range_high < range_low) {
		*prev = ret_node;
		tmp = rb_next(&ret_node->node);
		if (tmp) {
			*next = container_of(tmp, struct pmfs_range_node, node);
		} else {
			*next = NULL;
		}
	} else if (ret_node->range_low > range_high) {
		*next = ret_node;
		tmp = rb_prev(&ret_node->node);
		if (tmp) {
			*prev = container_of(tmp, struct pmfs_range_node, node);
		} else {
			*prev = NULL;
		}
	} else {
		pmfs_dbg("%s ERROR: %lu - %lu overlaps with existing "
			 "node %lu - %lu\n",
			 __func__, range_low, range_high, ret_node->range_low,
			 ret_node->range_high);
		return -EINVAL;
	}

	return 0;
}

static int pmfs_free_blocks(struct super_block *sb, unsigned long blocknr,
			    unsigned long num_blocks)
{
	struct rb_root *tree;
	unsigned long block_low;
	unsigned long block_high;
	struct pmfs_range_node *prev = NULL;
	struct pmfs_range_node *next = NULL;
	struct pmfs_range_node *curr_node;
	struct pmfs_free_list *free_list;
	int cpuid, socket;
	int new_node_used = 0;
	int ret;

	if (num_blocks <= 0) {
		pmfs_dbg("%s ERROR: free %lu\n", __func__, num_blocks);
		return -EINVAL;
	}

	pmfs_block_to_cpu_socket(sb, blocknr, &cpuid, &socket);

	/* Pre-allocate blocknode */
	curr_node = pmfs_alloc_blocknode(sb);
	if (curr_node == NULL) {
		/* returning without freeing the block*/
		return -ENOMEM;
	}

	free_list = pmfs_get_free_list(sb, cpuid, socket);
	spin_lock(&free_list->s_lock);

	tree = &(free_list->block_free_tree);

	block_low = blocknr;
	block_high = blocknr + num_blocks - 1;

	pmfs_dbg_alloc("Free: %lx - %lx to cpu: %d, socket: %d\n", block_low,
		       block_high, cpuid, socket);

	if (blocknr < free_list->block_start ||
	    blocknr + num_blocks > free_list->block_end + 1) {
		pmfs_err(sb,
			 "free blocks %lx to %lx, free list addr: %lx, "
			 "start %lu, end %lu\n",
			 blocknr, blocknr + num_blocks - 1,
			 (unsigned long)free_list, free_list->block_start,
			 free_list->block_end);
		ret = -EIO;
		goto out;
	}

	ret = pmfs_find_free_slot(tree, block_low, block_high, &prev, &next);

	if (ret) {
		pmfs_dbg("%s: find free slot fail: %d\n", __func__, ret);
		goto out;
	}

	if (prev && next && (block_low == prev->range_high + 1) &&
	    (block_high + 1 == next->range_low)) {
		/* fits the hole */
		rb_erase(&next->node, tree);
		free_list->num_blocknode--;
		prev->range_high = next->range_high;
		if (free_list->last_node == next)
			free_list->last_node = prev;
		pmfs_free_blocknode(next);
		goto block_found;
	}
	if (prev && (block_low == prev->range_high + 1)) {
		/* Aligns left */
		prev->range_high += num_blocks;
		goto block_found;
	}
	if (next && (block_high + 1 == next->range_low)) {
		/* Aligns right */
		next->range_low -= num_blocks;
		goto block_found;
	}

	/* Aligns somewhere in the middle */
	curr_node->range_low = block_low;
	curr_node->range_high = block_high;

	new_node_used = 1;
	ret = pmfs_insert_blocktree(tree, curr_node);
	if (ret) {
		new_node_used = 0;
		goto out;
	}
	if (!prev)
		free_list->first_node = curr_node;
	if (!next)
		free_list->last_node = curr_node;

	free_list->num_blocknode++;

block_found:
	free_list->num_free_blocks += num_blocks;

out:
	spin_unlock(&free_list->s_lock);
	if (new_node_used == 0)
		pmfs_free_blocknode(curr_node);

	return ret;
}

int pmfs_free_data_blocks(struct super_block *sb, unsigned long blocknr,
			  int num)
{
	int ret;

	if (blocknr == 0) {
		pmfs_dbg("%s: ERROR: %lu, %d\n", __func__, blocknr, num);
		return -EINVAL;
	}

	ret = pmfs_free_blocks(sb, blocknr, num);

	return ret;
}

int pmfs_free_index_blocks(struct super_block *sb, unsigned long blocknr,
			   int num)
{
	int ret;

	if (blocknr == 0) {
		pmfs_dbg("%s: ERROR: %lu, %d\n", __func__, blocknr, num);
		return -EINVAL;
	}

	ret = pmfs_free_blocks(sb, blocknr, num);

	return ret;
}

static int not_enough_blocks(struct pmfs_free_list *free_list,
			     unsigned long num_blocks)
{
	struct pmfs_range_node *first = free_list->first_node;
	struct pmfs_range_node *last = free_list->last_node;

	/*
   * free_list->num_free_blocks / free_list->num_blocknode is used to handle
   * fragmentation within blocknodes
   */
	if (!first || !last ||
	    free_list->num_free_blocks / free_list->num_blocknode <
		    num_blocks) {
		pmfs_dbg_alloc("%s: num_free_blocks=%ld; num_blocks=%ld; "
			       "first=0x%p; last=0x%p",
			       __func__, free_list->num_free_blocks, num_blocks,
			       first, last);
		return 1;
	}

	return 0;
}

/* Find out the free list with most free blocks */
static int pmfs_get_candidate_free_list(struct super_block *sb, int socket)
{
	struct pmfs_sb_info *sbi = PMFS_SB(sb);
	struct pmfs_free_list *free_list;
	int cpuid = 0;
	int num_free_blocks = 0;
	int i;

	for (i = 0; i < sbi->cpus; i++) {
		free_list = pmfs_get_free_list(sb, i, socket);
		if (free_list->num_free_blocks > num_free_blocks) {
			cpuid = i;
			num_free_blocks = free_list->num_free_blocks;
		}
	}

	return cpuid;
}

/* This method returns the number of blocks allocated. */
/* Return how many blocks allocated */
static long pmfs_alloc_blocks_in_free_list(struct super_block *sb,
					   struct pmfs_free_list *free_list,
					   unsigned long num_blocks,
					   unsigned long *new_blocknr)
{
	struct rb_root *tree;
	struct pmfs_range_node *curr, *next = NULL, *prev = NULL;
	struct rb_node *temp, *next_node, *prev_node;
	unsigned long curr_blocks;
	bool found = 0;

	unsigned long step = 0;

	if (!free_list->first_node || free_list->num_free_blocks == 0) {
		pmfs_dbg("%s: Can't alloc. free_list->first_node=0x%p "
			 "free_list->num_free_blocks = %lu",
			 __func__, free_list->first_node,
			 free_list->num_free_blocks);
		return -ENOSPC;
	}

	tree = &(free_list->block_free_tree);
	temp = &(free_list->first_node->node);

	/* always use the unaligned approach */
	while (temp) {
		step++;
		curr = container_of(temp, struct pmfs_range_node, node);

		curr_blocks = curr->range_high - curr->range_low + 1;

		if (num_blocks >= curr_blocks) {
			/* Superpage allocation must succeed */
			if (num_blocks > curr_blocks)
				goto next;

			/* Otherwise, allocate the whole blocknode */
			if (curr == free_list->first_node) {
				next_node = rb_next(temp);
				if (next_node)
					next = container_of(
						next_node,
						struct pmfs_range_node, node);
				free_list->first_node = next;
			}

			if (curr == free_list->last_node) {
				prev_node = rb_prev(temp);
				if (prev_node)
					prev = container_of(
						prev_node,
						struct pmfs_range_node, node);
				free_list->last_node = prev;
			}

			rb_erase(&curr->node, tree);
			free_list->num_blocknode--;
			num_blocks = curr_blocks;
			*new_blocknr = curr->range_low;
			pmfs_free_blocknode(curr);
			found = 1;
			break;
		}

		/* Allocate partial blocknode */

		*new_blocknr = curr->range_low;
		curr->range_low += num_blocks;

		found = 1;
		break;
	next:
		temp = rb_next(temp);
	}

	if (free_list->num_free_blocks < num_blocks) {
		pmfs_dbg("%s: free list has %lu free blocks, "
			 "but allocated %lu blocks?\n",
			 __func__, free_list->num_free_blocks, num_blocks);
		return -ENOSPC;
	}

	if (found == 1)
		free_list->num_free_blocks -= num_blocks;
	else {
		pmfs_dbg("%s: Can't alloc.  found = %d", __func__, found);
		return -ENOSPC;
	}

	return num_blocks;
}

int pmfs_new_blocks(struct super_block *sb, unsigned long *blocknr,
			   unsigned long num_blocks, int zero, int cpu,
			   int socket)
{
	struct pmfs_free_list *free_list;
	void *bp;
	unsigned long new_blocknr = 0;
	long ret_blocks = 0;
	int retried = 0;

	if (num_blocks == 0) {
		pmfs_dbg_verbose("%s: num_blocks == 0", __func__);
		return -EINVAL;
	}

retry:
	free_list = pmfs_get_free_list(sb, cpu, socket);
	spin_lock(&free_list->s_lock);

	if (not_enough_blocks(free_list, num_blocks)) {
		pmfs_dbg_alloc(
			"%s: cpu %d, socket: %d, free_blocks %lu, required %lu, "
			"blocknode %lu\n",
			__func__, cpu, socket, free_list->num_free_blocks,
			num_blocks, free_list->num_blocknode);

		if (retried >= 2)
			/* Allocate anyway */
			goto alloc;

		spin_unlock(&free_list->s_lock);
		cpu = pmfs_get_candidate_free_list(sb, socket);
		retried++;
		goto retry;
	}

alloc:
	ret_blocks = pmfs_alloc_blocks_in_free_list(sb, free_list, num_blocks,
						    &new_blocknr);

	spin_unlock(&free_list->s_lock);

	if (ret_blocks <= 0 || new_blocknr == 0) {
		pmfs_dbg("%s: not able to allocate %lu blocks. "
			 "ret_blocks=%ld; new_blocknr=%lu",
			 __func__, num_blocks, ret_blocks, new_blocknr);
		return -ENOSPC;
	}

	/* This should be delegated */
	if (zero) {

#if PMFS_DELEGATION_ENABLE
	    long pmfs_issued_cnt[PMFS_MAX_SOCKET];
	    struct pmfs_notifyer pmfs_completed_cnt[PMFS_MAX_SOCKET];
        struct pmfs_sb_info * sbi = PMFS_SB(sb);
#endif


		bp = pmfs_get_virt_addr_from_offset(
			sb, pmfs_get_block_off(sb, new_blocknr, 0));

#if PMFS_DELEGATION_ENABLE
		if (sbi->delegation_ready &&
		        PAGE_SIZE * ret_blocks > PMFS_WRITE_DELEGATION_LIMIT) {

		    memset(pmfs_issued_cnt, 0, sizeof(long) * PMFS_MAX_SOCKET);
		    memset(pmfs_completed_cnt, 0, sizeof(struct pmfs_notifyer) *
		            PMFS_MAX_SOCKET);

            pmfs_do_write_delegation(sbi, NULL, 0, (unsigned long) bp,
                    PAGE_SIZE * ret_blocks, 1, 1, 0, pmfs_issued_cnt,
                    pmfs_completed_cnt, 0);

            pmfs_complete_delegation(pmfs_issued_cnt, pmfs_completed_cnt);
		}
		else  {

		    pmfs_memunlock_range(sb, bp, PAGE_SIZE * ret_blocks);
		    memset_nt(bp, 0, PAGE_SIZE * ret_blocks);
		    pmfs_memlock_range(sb, bp, PAGE_SIZE * ret_blocks);

	        /* quick fix for the cpu soft lockup during fallocate */
	        if (need_resched())
	            cond_resched();
		}
#else
        pmfs_memunlock_range(sb, bp, PAGE_SIZE * ret_blocks);
        memset_nt(bp, 0, PAGE_SIZE * ret_blocks);
        pmfs_memlock_range(sb, bp, PAGE_SIZE * ret_blocks);

        /* quick fix for the cpu soft lockup during fallocate */
        if (need_resched())
            cond_resched();
#endif

	}
	*blocknr = new_blocknr;

	pmfs_dbg_alloc("Alloc %lu NVMM blocks 0x%lx at cpu: %d, socket: %d\n",
		       ret_blocks, *blocknr, cpu, socket);
	return ret_blocks;
}

/*
 * These are the functions to call the allocator to get blocks. Let's also
 * make decisions based on allocation functions in these functions
 */

/*
 * Allocate data blocks; blocks to hold file data
 * The offset for the allocated block comes back in
 * blocknr.  Return the number of blocks allocated.
 */

static int pmfs_do_new_data_blocks(struct super_block *sb,
				   struct pmfs_inode *inode,
				   unsigned long *blocknr, unsigned int num,
				   int zero, int curr_cpu)
{
	int allocated;

	int num_blocks = num * pmfs_get_numblocks(inode->i_blk_type);

	allocated = pmfs_new_blocks(sb, blocknr, num_blocks, zero, curr_cpu,
				    inode->nsocket);

	inode->nsocket = pmfs_get_nsocket(sb, inode);

	return allocated;
}

/*
 * Allocate index blocks; blocks to hold the indexing data of a file.
 * The offset for the allocated block comes back in
 * blocknr.  Return the number of blocks allocated.
 */
static int pmfs_do_new_index_blocks(struct super_block *sb,
				    struct pmfs_inode *inode,
				    unsigned long *blocknr, unsigned int num,
				    int zero, int curr_cpu)
{
	int allocated, socket;

	/*
   * This is one policy, use the socket of the curr_cpu. The other policy
   * may be record the numa of the cpu that creates the inode and just use
   * that
   */

	socket = cpu_to_node(curr_cpu);

	allocated = pmfs_new_blocks(sb, blocknr, num, zero, curr_cpu, socket);

	return allocated;
}

/*
 * allocate a data block for inode and return it's absolute blocknr.
 * Zeroes out the block if zero set. Increments inode->i_blocks.
 */
int pmfs_new_data_blocks(struct super_block *sb, struct pmfs_inode *pi,
			 unsigned long *blocknr, int zero)
{
	unsigned int data_bits = blk_type_to_shift_pmfs[pi->i_blk_type];

	int allocated = pmfs_do_new_data_blocks(sb, pi, blocknr, 1, zero,
						smp_processor_id());

	if (allocated > 0) {
		pmfs_memunlock_inode(sb, pi);
		le64_add_cpu(&pi->i_blocks,
			     (1 << (data_bits - sb->s_blocksize_bits)));
		pmfs_memlock_inode(sb, pi);
	}

	return allocated > 0 ? 0 : -1;
}

int pmfs_new_index_blocks(struct super_block *sb, struct pmfs_inode *inode,
			  unsigned long *blocknr, int zero)
{
	int allocated = pmfs_do_new_index_blocks(sb, inode, blocknr, 1, zero,
						 smp_processor_id());

	return allocated > 0 ? 0 : -1;
}




unsigned long pmfs_count_free_blocks(struct super_block *sb)
{
	struct pmfs_sb_info *sbi = PMFS_SB(sb);
	struct pmfs_free_list *free_list;
	unsigned long num_free_blocks = 0;
	int i, j;

	for (i = 0; i < sbi->cpus; i++)
		for (j = 0; j < sbi->sockets; j++) {
			free_list = pmfs_get_free_list(sb, i, j);
			num_free_blocks += free_list->num_free_blocks;
		}

	return num_free_blocks;
}
