/*
 * BRIEF DESCRIPTION
 *
 * Inode operations for directories.
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
#include <linux/pagemap.h>
#include "winefs.h"
#include "xip.h"
#include "xattr.h"
#include "inode.h"

/*
 * Couple of helper functions - make the code slightly cleaner.
 */
static inline void winefs_inc_count(struct inode *inode, struct winefs_inode *pi)
{
	inc_nlink(inode);
	winefs_update_nlink(inode, pi);
}

static inline void winefs_dec_count(struct inode *inode, struct winefs_inode *pi)
{
	if (inode->i_nlink) {
		drop_nlink(inode);
		winefs_update_nlink(inode, pi);
	}
}

static inline int winefs_add_nondir(winefs_transaction_t *trans,
		struct inode *dir, struct dentry *dentry, struct inode *inode)
{
	struct winefs_inode *pi;
	int err = winefs_add_entry(trans, dentry, inode);

	if (!err) {
		d_instantiate(dentry, inode);
		unlock_new_inode(inode);
		return 0;
	}
	pi = winefs_get_inode(inode->i_sb, inode->i_ino);

	winefs_dec_count(inode, pi);
	unlock_new_inode(inode);
	iput(inode);
	return err;
}

static inline struct winefs_direntry *winefs_next_entry(struct winefs_direntry *p)
{
	return (struct winefs_direntry *)((char *)p + le16_to_cpu(p->de_len));
}

/*
 * Methods themselves.
 */
int winefs_check_dir_entry(const char *function, struct inode *dir,
			  struct winefs_direntry *de, u8 *base,
			  unsigned long offset)
{
	const char *error_msg = NULL;

	const int rlen = le16_to_cpu(de->de_len);

	if (unlikely(rlen < WINEFS_DIR_REC_LEN(1)))
		error_msg = "de_len is smaller than minimal";
	else if (unlikely(rlen % 4 != 0))
		error_msg = "de_len % 4 != 0";
	else if (unlikely(rlen < WINEFS_DIR_REC_LEN(de->name_len)))
		error_msg = "de_len is too small for name_len";
	else if (unlikely((((u8 *)de - base) + rlen > dir->i_sb->s_blocksize))) {
		error_msg = "directory entry across blocks";
		dump_stack();
	}

	if (unlikely(error_msg != NULL)) {
		winefs_dbg("bad entry in directory #%lu: %s - "
			  "offset=%lu, inode=%lu, rec_len=%d, name_len=%d",
			  dir->i_ino, error_msg, offset,
			  (unsigned long)le64_to_cpu(de->ino), rlen,
			  de->name_len);
	}

	return error_msg == NULL ? 1 : 0;
}

static ino_t winefs_inode_by_name(struct inode *dir, struct qstr *entry,
				 struct winefs_direntry **res_entry)
{
	struct super_block *sb = dir->i_sb;
	struct winefs_direntry *direntry;

	direntry = winefs_find_dentry(sb, NULL, dir,
				    entry->name, entry->len);
	if (direntry == NULL) {
		return 0;
	}

	*res_entry = direntry;
	return direntry->ino;
}

static struct dentry *winefs_lookup(struct inode *dir, struct dentry *dentry,
				   unsigned int flags)
{
	struct inode *inode = NULL;
	struct winefs_direntry *de;
	ino_t ino;

	if (dentry->d_name.len > WINEFS_NAME_LEN)
		return ERR_PTR(-ENAMETOOLONG);

	ino = winefs_inode_by_name(dir, &dentry->d_name, &de);
	if (ino) {
		inode = winefs_iget(dir->i_sb, ino);
		if (inode == ERR_PTR(-ESTALE)) {
			winefs_err(dir->i_sb, "%s: deleted inode referenced: %lu",
				  __func__, (unsigned long)ino);
			return ERR_PTR(-EIO);
		}
	}

	return d_splice_alias(inode, dentry);
}

/*
 * By the time this is called, we already have created
 * the directory cache entry for the new file, but it
 * is so far negative - it has no inode.
 *
 * If the create succeeds, we fill in the inode information
 * with d_instantiate().
 */
static int winefs_create(struct user_namespace *mnt_userns, struct inode *dir,
			 struct dentry *dentry, umode_t mode, bool excl)
{
	struct inode *inode = NULL;
	int err = PTR_ERR(inode);
	struct super_block *sb = dir->i_sb;
	winefs_transaction_t *trans;
	timing_t create_time, new_inode_time, add_nondir_time, new_trans_time,
	  commit_trans_time;

	WINEFS_START_TIMING(create_t, create_time);
	/* two log entries for new inode, 1 lentry for dir inode, 1 for dir
	 * inode's b-tree, 2 lentries for logging dir entry
	 */

	WINEFS_START_TIMING(create_new_trans_t, new_trans_time);
	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 2 +
				     MAX_DIRENTRY_LENTRIES, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		err = PTR_ERR(trans);
		goto out;
	}
	WINEFS_END_TIMING(create_new_trans_t, new_trans_time);

	WINEFS_START_TIMING(new_inode_t, new_inode_time);
	inode = winefs_new_inode(trans, dir, mode, &dentry->d_name);
	WINEFS_END_TIMING(new_inode_t, new_inode_time);
	if (IS_ERR(inode))
		goto out_err;
	winefs_dbg_verbose("%s: %s, ino %lu\n", __func__,
				dentry->d_name.name, inode->i_ino);
	inode->i_op = &winefs_file_inode_operations;
	inode->i_mapping->a_ops = &winefs_aops_xip;
	inode->i_fop = &winefs_xip_file_operations;
	WINEFS_START_TIMING(add_nondir_t, add_nondir_time);
	err = winefs_add_nondir(trans, dir, dentry, inode);
	WINEFS_END_TIMING(add_nondir_t, add_nondir_time);
	if (err)
		goto out_err;
	WINEFS_START_TIMING(create_commit_trans_t, commit_trans_time);
	winefs_commit_transaction(sb, trans);
	WINEFS_END_TIMING(create_commit_trans_t, commit_trans_time);
out:
	WINEFS_END_TIMING(create_t, create_time);
	winefs_dbg_verbose("%s: successful. Return value = %d\n", __func__, err);
	return err;
out_err:
	winefs_abort_transaction(sb, trans);
	return err;
}

static int winefs_mknod(struct user_namespace *mnt_userns, struct inode *dir,
			struct dentry *dentry, umode_t mode, dev_t rdev)
{
	struct inode *inode = NULL;
	int err = PTR_ERR(inode);
	winefs_transaction_t *trans;
	struct super_block *sb = dir->i_sb;
	struct winefs_inode *pi;

	/* 2 log entries for new inode, 1 lentry for dir inode, 1 for dir
	 * inode's b-tree, 2 lentries for logging dir entry
	 */
	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 2 +
			MAX_DIRENTRY_LENTRIES, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		err = PTR_ERR(trans);
		goto out;
	}

	inode = winefs_new_inode(trans, dir, mode, &dentry->d_name);
	if (IS_ERR(inode))
		goto out_err;
	init_special_inode(inode, mode, rdev);
	inode->i_op = &winefs_special_inode_operations;

	pi = winefs_get_inode(sb, inode->i_ino);
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
		pi->dev.rdev = cpu_to_le32(inode->i_rdev);
	err = winefs_add_nondir(trans, dir, dentry, inode);
	if (err)
		goto out_err;
	winefs_commit_transaction(sb, trans);
out:
	return err;
out_err:
	winefs_abort_transaction(sb, trans);
	return err;
}

static int winefs_symlink(struct user_namespace *mnt_userns, struct inode *dir,
			  struct dentry *dentry, const char *symname)
{
	struct super_block *sb = dir->i_sb;
	int err = -ENAMETOOLONG;
	unsigned len = strlen(symname);
	struct inode *inode;
	winefs_transaction_t *trans;
	struct winefs_inode *pi;

	if (len + 1 > sb->s_blocksize)
		goto out;

	/* 2 log entries for new inode, 1 lentry for dir inode, 1 for dir
	 * inode's b-tree, 2 lentries for logging dir entry
	 */
	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 2 +
			MAX_DIRENTRY_LENTRIES, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		err = PTR_ERR(trans);
		goto out;
	}

	inode = winefs_new_inode(trans, dir, S_IFLNK|S_IRWXUGO, &dentry->d_name);
	err = PTR_ERR(inode);
	if (IS_ERR(inode)) {
		winefs_abort_transaction(sb, trans);
		goto out;
	}

	inode->i_op = &winefs_symlink_inode_operations;
	inode->i_mapping->a_ops = &winefs_aops_xip;

	pi = winefs_get_inode(sb, inode->i_ino);
	err = winefs_block_symlink(inode, symname, len);
	if (err)
		goto out_fail;

	inode->i_size = len;
	winefs_update_isize(inode, pi);

	err = winefs_add_nondir(trans, dir, dentry, inode);
	if (err) {
		winefs_abort_transaction(sb, trans);
		goto out;
	}

	winefs_commit_transaction(sb, trans);
out:
	return err;

out_fail:
	winefs_dec_count(inode, pi);
	unlock_new_inode(inode);
	iput(inode);
	winefs_abort_transaction(sb, trans);
	goto out;
}

static int winefs_link(struct dentry *dest_dentry, struct inode *dir,
		      struct dentry *dentry)
{
	struct inode *inode = dest_dentry->d_inode;
	int err = -ENOMEM;
	winefs_transaction_t *trans;
	struct super_block *sb = inode->i_sb;
	struct winefs_inode *pi = winefs_get_inode(sb, inode->i_ino);

	if (inode->i_nlink >= WINEFS_LINK_MAX)
		return -EMLINK;

	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 2 +
			MAX_DIRENTRY_LENTRIES, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		err = PTR_ERR(trans);
		goto out;
	}
	/* only need to log the first 48 bytes since we only modify ctime and
	 * i_links_count in this system call */
	winefs_add_logentry(sb, trans, pi, MAX_DATA_PER_LENTRY, LE_DATA);

	ihold(inode);

	err = winefs_add_entry(trans, dentry, inode);
	if (!err) {
		inode->i_ctime = current_time(inode);
		inc_nlink(inode);

		winefs_memunlock_inode(sb, pi);
		pi->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
		pi->i_links_count = cpu_to_le16(inode->i_nlink);
		winefs_memlock_inode(sb, pi);

		d_instantiate(dentry, inode);
		winefs_commit_transaction(sb, trans);
	} else {
		iput(inode);
		winefs_abort_transaction(sb, trans);
	}
out:
	return err;
}

static int winefs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	int retval = -ENOMEM;
	winefs_transaction_t *trans;
	struct super_block *sb = inode->i_sb;
	struct winefs_inode *pi = winefs_get_inode(sb, inode->i_ino);
	timing_t unlink_time, remove_entry_time, unlink_new_trans_time,
	  unlink_commit_trans_time, truncate_add_time;

	WINEFS_START_TIMING(unlink_t, unlink_time);

	WINEFS_START_TIMING(unlink_new_trans_t, unlink_new_trans_time);
	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 2 +
		MAX_DIRENTRY_LENTRIES, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		retval = PTR_ERR(trans);
		goto out;
	}
	winefs_add_logentry(sb, trans, pi, MAX_DATA_PER_LENTRY, LE_DATA);
	WINEFS_END_TIMING(unlink_new_trans_t, unlink_new_trans_time);

	winefs_dbg_verbose("%s: %s, ino %lu\n", __func__,
				dentry->d_name.name, inode->i_ino);

	WINEFS_START_TIMING(remove_entry_t, remove_entry_time);
	retval = winefs_remove_entry(trans, dentry, inode);
	WINEFS_END_TIMING(remove_entry_t, remove_entry_time);
	if (retval)
		goto end_unlink;

	WINEFS_START_TIMING(truncate_add_t, truncate_add_time);
	if (inode->i_nlink == 1)
		winefs_truncate_add(inode, inode->i_size);
	inode->i_ctime = dir->i_ctime;
	WINEFS_END_TIMING(truncate_add_t, truncate_add_time);

	winefs_memunlock_inode(sb, pi);
	if (inode->i_nlink) {
		drop_nlink(inode);
		pi->i_links_count = cpu_to_le16(inode->i_nlink);
	}
	pi->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
	winefs_memlock_inode(sb, pi);

	WINEFS_START_TIMING(unlink_commit_trans_t, unlink_commit_trans_time);
	winefs_commit_transaction(sb, trans);
	WINEFS_END_TIMING(unlink_commit_trans_t, unlink_commit_trans_time);
	WINEFS_END_TIMING(unlink_t, unlink_time);

	return 0;
end_unlink:
	winefs_abort_transaction(sb, trans);
out:
	return retval;
}

static int winefs_mkdir(struct user_namespace *mnt_userns, struct inode *dir,
			struct dentry *dentry, umode_t mode)
{
	struct inode *inode;
	struct winefs_inode *pi, *pidir;
	struct winefs_direntry *de = NULL;
	struct super_block *sb = dir->i_sb;
	winefs_transaction_t *trans;
	int err = -EMLINK;
	char *blk_base;
	u64 bp = 0;

	if (dir->i_nlink >= WINEFS_LINK_MAX)
		goto out;

	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 2 +
			MAX_DIRENTRY_LENTRIES, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		err = PTR_ERR(trans);
		goto out;
	}

	inode = winefs_new_inode(trans, dir, S_IFDIR | mode, &dentry->d_name);
	err = PTR_ERR(inode);
	if (IS_ERR(inode)) {
		winefs_abort_transaction(sb, trans);
		goto out;
	}

	winefs_dbg_verbose("%s: %s, ino %lu\n", __func__,
				dentry->d_name.name, inode->i_ino);
	inode->i_op = &winefs_dir_inode_operations;
	inode->i_fop = &winefs_dir_operations;
	inode->i_mapping->a_ops = &winefs_aops_xip;

	/* since this is a new inode so we don't need to include this
	 * winefs_alloc_blocks in the transaction
	 */
	err = winefs_alloc_blocks_weak(NULL, inode, 0, 1,
				false, ANY_CPU, 0);
	if (err)
		goto out_clear_inode;
	inode->i_size = sb->s_blocksize;

	winefs_find_data_blocks(inode, 0, &bp, 1);
	blk_base = winefs_get_block(sb, bp);
	de = (struct winefs_direntry *)blk_base;
	winefs_memunlock_range(sb, blk_base, sb->s_blocksize);
	de->ino = cpu_to_le64(inode->i_ino);
	de->name_len = 1;
	de->de_len = cpu_to_le16(WINEFS_DIR_REC_LEN(de->name_len));
	strcpy(de->name, ".");
	/*de->file_type = S_IFDIR; */
	de = winefs_next_entry(de);
	de->ino = cpu_to_le64(dir->i_ino);
	de->de_len = cpu_to_le16(sb->s_blocksize - WINEFS_DIR_REC_LEN(1));
	de->name_len = 2;
	strcpy(de->name, "..");
	/*de->file_type =  S_IFDIR; */
	winefs_memlock_range(sb, blk_base, sb->s_blocksize);

	/* No need to journal the dir entries but we need to persist them */
	winefs_flush_buffer(blk_base, WINEFS_DIR_REC_LEN(1) +
			WINEFS_DIR_REC_LEN(2), true);

	set_nlink(inode, 2);

	err = winefs_add_entry(trans, dentry, inode);
	if (err) {
		winefs_dbg_verbose("failed to add dir entry\n");
		goto out_clear_inode;
	}
	pi = winefs_get_inode(sb, inode->i_ino);
	winefs_memunlock_inode(sb, pi);
	pi->i_links_count = cpu_to_le16(inode->i_nlink);
	pi->i_size = cpu_to_le64(inode->i_size);
	winefs_memlock_inode(sb, pi);

	pidir = winefs_get_inode(sb, dir->i_ino);
	winefs_inc_count(dir, pidir);
	d_instantiate(dentry, inode);
	unlock_new_inode(inode);

	winefs_commit_transaction(sb, trans);

out:
	return err;

out_clear_inode:
	clear_nlink(inode);
	unlock_new_inode(inode);
	iput(inode);
	winefs_abort_transaction(sb, trans);
	goto out;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
static int winefs_empty_dir(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct winefs_inode_info *si = WINEFS_I(inode);
	struct winefs_inode_info_header *sih = &si->header;
	struct winefs_range_node *curr;
	struct winefs_direntry *entry;
	struct rb_node *temp;

	temp = rb_first(&sih->rb_tree);
	while (temp) {
		curr = container_of(temp, struct winefs_range_node, node);
		entry = curr->direntry;

		if (!is_dir_init_entry(sb, entry))
			return 0;

		temp = rb_next(temp);
	}

	return 1;
}

static int winefs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct winefs_direntry *de;
	winefs_transaction_t *trans;
	struct super_block *sb = inode->i_sb;
	struct winefs_inode *pi = winefs_get_inode(sb, inode->i_ino), *pidir;
	int err = -ENOTEMPTY;

	if (!inode)
		return -ENOENT;

	winefs_dbg_verbose("%s: %s, ino %lu\n", __func__,
				dentry->d_name.name, inode->i_ino);
	if (winefs_inode_by_name(dir, &dentry->d_name, &de) == 0)
		return -ENOENT;

	if (!winefs_empty_dir(inode))
		return err;

	if (inode->i_nlink != 2)
		winefs_dbg("empty directory has nlink!=2 (%d)", inode->i_nlink);

	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 2 +
			MAX_DIRENTRY_LENTRIES, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		err = PTR_ERR(trans);
		return err;
	}
	winefs_add_logentry(sb, trans, pi, MAX_DATA_PER_LENTRY, LE_DATA);

	err = winefs_remove_entry(trans, dentry, inode);
	if (err)
		goto end_rmdir;

	/*inode->i_version++; */
	clear_nlink(inode);
	inode->i_ctime = dir->i_ctime;

	winefs_memunlock_inode(sb, pi);
	pi->i_links_count = cpu_to_le16(inode->i_nlink);
	pi->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
	winefs_memlock_inode(sb, pi);

	/* add the inode to truncate list in case a crash happens before the
	 * subsequent evict_inode is called. It will be deleted from the
	 * truncate list during evict_inode.
	 */
	winefs_truncate_add(inode, inode->i_size);

	pidir = winefs_get_inode(sb, dir->i_ino);
	winefs_dec_count(dir, pidir);

	winefs_commit_transaction(sb, trans);
	return err;
end_rmdir:
	winefs_abort_transaction(sb, trans);
	return err;
}

static int winefs_rename(struct user_namespace *mnt_userns, struct inode *old_dir,
			 struct dentry *old_dentry,
			 struct inode *new_dir, struct dentry *new_dentry,
			 unsigned int flags)
{
	struct inode *old_inode = old_dentry->d_inode;
	struct inode *new_inode = new_dentry->d_inode;
	struct winefs_direntry *new_de = NULL, *old_de = NULL;
	winefs_transaction_t *trans;
	struct super_block *sb = old_inode->i_sb;
	struct winefs_inode *pi, *new_pidir, *old_pidir;
	int err = -ENOENT;

	winefs_inode_by_name(new_dir, &new_dentry->d_name, &new_de);
	winefs_inode_by_name(old_dir, &old_dentry->d_name, &old_de);

	winefs_dbg_verbose("%s: rename %s to %s\n", __func__,
			old_dentry->d_name.name, new_dentry->d_name.name);
	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES * 4 +
			MAX_DIRENTRY_LENTRIES * 2, winefs_get_cpuid(sb));
	if (IS_ERR(trans)) {
		return PTR_ERR(trans);
	}

	if (new_inode) {
		err = -ENOTEMPTY;
		if (S_ISDIR(old_inode->i_mode) && !winefs_empty_dir(new_inode))
			goto out;
	} else {
		if (S_ISDIR(old_inode->i_mode)) {
			err = -EMLINK;
			if (new_dir->i_nlink >= WINEFS_LINK_MAX)
				goto out;
		}
	}

	new_pidir = winefs_get_inode(sb, new_dir->i_ino);

	pi = winefs_get_inode(sb, old_inode->i_ino);
	old_inode->i_ctime = current_time(old_inode);
	winefs_add_logentry(sb, trans, pi, MAX_DATA_PER_LENTRY, LE_DATA);

	if (!new_de) {
		/* link it into the new directory. */
		err = winefs_add_entry(trans, new_dentry, old_inode);
		if (err)
			goto out;
	} else {
		winefs_add_logentry(sb, trans, &new_de->ino, sizeof(new_de->ino),
			LE_DATA);

		winefs_memunlock_range(sb, new_de, sb->s_blocksize);
		new_de->ino = cpu_to_le64(old_inode->i_ino);
		/*new_de->file_type = old_de->file_type; */
		winefs_memlock_range(sb, new_de, sb->s_blocksize);

		winefs_add_logentry(sb, trans, new_pidir, MAX_DATA_PER_LENTRY,
			LE_DATA);
		/*new_dir->i_version++; */
		new_dir->i_ctime = new_dir->i_mtime = current_time(new_dir);
		winefs_update_time(new_dir, new_pidir);
	}

	/* and unlink the inode from the old directory ... */
	err = winefs_remove_entry(trans, old_dentry, old_inode);
	if (err)
		goto out;

	if (new_inode) {
		pi = winefs_get_inode(sb, new_inode->i_ino);
		winefs_add_logentry(sb, trans, pi, MAX_DATA_PER_LENTRY, LE_DATA);
		new_inode->i_ctime = current_time(new_inode);

		winefs_memunlock_inode(sb, pi);
		if (S_ISDIR(old_inode->i_mode)) {
			if (new_inode->i_nlink)
				drop_nlink(new_inode);
		}
		pi->i_ctime = cpu_to_le32(new_inode->i_ctime.tv_sec);
		if (new_inode->i_nlink)
			drop_nlink(new_inode);
		pi->i_links_count = cpu_to_le16(new_inode->i_nlink);
		winefs_memlock_inode(sb, pi);

		if (!new_inode->i_nlink)
			winefs_truncate_add(new_inode, new_inode->i_size);
	} else {
		if (S_ISDIR(old_inode->i_mode)) {
			winefs_inc_count(new_dir, new_pidir);
			old_pidir = winefs_get_inode(sb, old_dir->i_ino);
			winefs_dec_count(old_dir, old_pidir);
		}
	}

	winefs_commit_transaction(sb, trans);
	return 0;
out:
	winefs_abort_transaction(sb, trans);
	return err;
}

struct dentry *winefs_get_parent(struct dentry *child)
{
	struct inode *inode;
	struct qstr dotdot = QSTR_INIT("..", 2);
	struct winefs_direntry *de = NULL;
	ino_t ino;

	winefs_inode_by_name(child->d_inode, &dotdot, &de);
	if (!de)
		return ERR_PTR(-ENOENT);
	ino = le64_to_cpu(de->ino);

	if (ino)
		inode = winefs_iget(child->d_inode->i_sb, ino);
	else
		return ERR_PTR(-ENOENT);

	return d_obtain_alias(inode);
}

const struct inode_operations winefs_dir_inode_operations = {
	.create		= winefs_create,
	.lookup		= winefs_lookup,
	.link		= winefs_link,
	.unlink		= winefs_unlink,
	.symlink	= winefs_symlink,
	.mkdir		= winefs_mkdir,
	.rmdir		= winefs_rmdir,
	.mknod		= winefs_mknod,
	.rename		= winefs_rename,
	.setattr	= winefs_notify_change,
	.listxattr      = winefs_listxattr,
	.get_acl	= NULL,
};

const struct inode_operations winefs_special_inode_operations = {
	.setattr	= winefs_notify_change,
	.get_acl	= NULL,
};
