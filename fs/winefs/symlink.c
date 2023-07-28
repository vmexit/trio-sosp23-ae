/*
 * BRIEF DESCRIPTION
 *
 * Symlink operations
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

#include <linux/fs.h>
#include <linux/namei.h>
#include "winefs.h"
#include "inode.h"

int winefs_block_symlink(struct inode *inode, const char *symname, int len)
{
	struct super_block *sb = inode->i_sb;
	u64 block;
	char *blockp;
	int err;

	err = winefs_alloc_blocks_weak(NULL, inode, 0, 1,
				false, ANY_CPU, 0);
	if (err)
		return err;

	winefs_find_data_blocks(inode, 0, &block, 1);
	blockp = winefs_get_block(sb, block);

	winefs_memunlock_block(sb, blockp);
	memcpy(blockp, symname, len);
	blockp[len] = '\0';
	winefs_memlock_block(sb, blockp);
	winefs_flush_buffer(blockp, len+1, false);
	return 0;
}

/* FIXME: Temporary workaround */
static int winefs_readlink_copy(char __user *buffer, int buflen, const char *link)
{
	int len = PTR_ERR(link);
	if (IS_ERR(link))
		goto out;

	len = strlen(link);
	if (len > (unsigned) buflen)
		len = buflen;
	if (copy_to_user(buffer, link, len))
		len = -EFAULT;
out:
	return len;
}

static int winefs_readlink(struct dentry *dentry, char __user *buffer, int buflen)
{
	struct inode *inode = dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	u64 block;
	char *blockp;

	winefs_find_data_blocks(inode, 0, &block, 1);
	blockp = winefs_get_block(sb, block);
	return winefs_readlink_copy(buffer, buflen, blockp);
}

static const char *winefs_get_link(struct dentry *dentry, struct inode *inode,
	struct delayed_call *done)
{
	struct super_block *sb = inode->i_sb;
	u64 block;
	char *blockp;

	winefs_find_data_blocks(inode, 0, &block, 1);
	blockp = winefs_get_block(sb, block);
	return blockp;
}

const struct inode_operations winefs_symlink_inode_operations = {
	.readlink	= winefs_readlink,
	.get_link	= winefs_get_link,
	.setattr	= winefs_notify_change,
};
