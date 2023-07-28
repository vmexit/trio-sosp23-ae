#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mbcache.h>
#include <linux/quotaops.h>
#include <linux/iversion.h>
#include "winefs.h"
#include "xattr.h"
#include "inode.h"

const struct xattr_handler *winefs_xattr_handlers[] = {
	&winefs_xattr_user_handler,
	NULL
};

static inline void
winefs_update_special_xattr(struct super_block *sb, void *pmem_addr, struct winefs_special_xattr_info *xattr_info)
{
	if (pmem_addr && xattr_info) {
		winefs_memunlock_range(sb, pmem_addr, sizeof(struct winefs_special_xattr_info));
		memcpy(pmem_addr, (void *)xattr_info, sizeof(struct winefs_special_xattr_info));
		winefs_flush_buffer((void *)xattr_info, sizeof(struct winefs_special_xattr_info), false);
		winefs_memlock_range(sb, pmem_addr, sizeof(struct winefs_special_xattr_info));
	}
}

int
winefs_xattr_set(struct inode *inode, const char *name,
	       const void *value, size_t value_len, int flags)
{
	winefs_transaction_t *trans;
	struct winefs_inode *pi;
	int cpu;
	struct super_block *sb = inode->i_sb;
	const char *special_xattr = "file_type";
	const char *special_xattr_value_mmap = "mmap";
	const char *special_xattr_value_sys = "sys";
	u64 bp;
	int ret = 0;
	unsigned long blocknr = 0;
	int num_blocks = 0;
	struct winefs_special_xattr_info xattr_info;

	winefs_dbg("%s: start\n", __func__);
	cpu = winefs_get_cpuid(sb);

	if (memcmp(name, special_xattr, strlen(special_xattr))) {
		winefs_dbg("checking name. name = %s, special_xattr = %s, strlen(special_xattr) = %lu\n",
			 name, special_xattr, strlen(special_xattr));
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (memcmp(value, (void *)special_xattr_value_mmap, value_len) &&
	    memcmp(value, (void *)special_xattr_value_sys, value_len)) {
		winefs_dbg("value = %s, special_xattr_value_mmap = %s, value_len = %lu\n",
			 value, special_xattr_value_mmap, value_len);
		ret = -EOPNOTSUPP;
		goto out;
	}

	pi = winefs_get_inode(sb, inode->i_ino);
	trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES, cpu);

	if (!pi->i_xattr) {
		winefs_add_logentry(sb, trans, pi, sizeof(*pi), LE_DATA);
		num_blocks = winefs_new_blocks(sb, &blocknr, 1, WINEFS_BLOCK_TYPE_4K, 1, cpu);
		if (num_blocks == 0) {
			ret = -ENOSPC;
			winefs_abort_transaction(sb, trans);
			goto out;
		}
		winefs_memunlock_range(sb, pi, CACHELINE_SIZE);
		pi->i_xattr = winefs_get_block_off(sb, blocknr, WINEFS_BLOCK_TYPE_4K);
		winefs_memlock_range(sb, pi, CACHELINE_SIZE);
	}

	xattr_info.name = WINEFS_SPECIAL_XATTR_NAME;
	if (!memcmp(value, (void *)special_xattr_value_mmap, value_len)) {
		xattr_info.value = WINEFS_SPECIAL_XATTR_MMAP_VALUE;
		pi->huge_aligned_file = 1;
	} else {
		xattr_info.value = WINEFS_SPECIAL_XATTR_SYS_VALUE;
		pi->huge_aligned_file = 0;
	}

	bp = winefs_get_block(sb, pi->i_xattr);
	winefs_update_special_xattr(sb, (void *)bp, &xattr_info);
	winefs_commit_transaction(sb, trans);

 out:
	return ret;
}

ssize_t
winefs_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size)
{
	int ret = 0;
	struct inode *inode = d_inode(dentry);
	struct super_block *sb = inode->i_sb;
	struct winefs_inode *pi = winefs_get_inode(sb, inode->i_ino);
	const char *special_xattr = "file_type";
	const char *user_special_xattr = "user.file_type";
	int fraction_of_hugepage_files = 0;

	inode_lock(inode);


	/* [TODO]: List through the inodes of all the files if dir
	 * and mark dir as hugepage aligned if all the files
	 * in the dir are hugepage aligned
	 */
	if (S_ISDIR(inode->i_mode)) {
		fraction_of_hugepage_files = winefs_get_ratio_hugepage_files_in_dir(sb, inode);
		if (fraction_of_hugepage_files == 1) {
			pi->huge_aligned_file = 1;
		}
	}

	if (pi->huge_aligned_file && !pi->i_xattr) {
		ret = winefs_xattr_set(inode, special_xattr, "mmap", 4, 0);
		if (ret != 0) {
			goto out;
		}
	}

	if (!pi->i_xattr) {
		ret = 0;
		goto out;
	}

	if (buffer && buffer_size > strlen(user_special_xattr)) {
		memcpy(buffer, user_special_xattr, strlen(user_special_xattr));
		buffer += strlen(user_special_xattr);
		ret += strlen(user_special_xattr);
		*buffer++ = 0;
		ret += 1;
	} else if (buffer) {
		ret = -ERANGE;
	} else {
		ret = strlen(user_special_xattr) + 1;
	}

 out:
	inode_unlock(inode);
	return ret;
}

int winefs_xattr_get(struct inode *inode, const char *name,
		   void *buffer, size_t size)
{
	struct winefs_inode *pi;
	struct super_block *sb = inode->i_sb;
	const char *special_xattr = "file_type";
	const char *special_xattr_value_mmap = "mmap";
	const char *special_xattr_value_sys = "sys";
	u64 bp;
	int ret = 0;
	int value = 0;

	inode_lock(inode);

	if (memcmp(name, special_xattr, strlen(special_xattr))) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	pi = winefs_get_inode(sb, inode->i_ino);

	if (pi->huge_aligned_file && !pi->i_xattr) {
		ret = winefs_xattr_set(inode, special_xattr, "mmap", 4, 0);
		if (ret != 0) {
			goto out;
		}
	}

	if (!pi->i_xattr) {
		ret = -ENODATA;
		goto out;
	}

	bp = winefs_get_block(sb, pi->i_xattr);
	memcpy(&value, (void *) (bp + sizeof(int)), sizeof(int));
	winefs_dbg_verbose("%s: value = %d. buffer size = %lu\n", __func__, value, size);
	if (buffer) {
		if (value == WINEFS_SPECIAL_XATTR_MMAP_VALUE && size >= 4) {
			memcpy(buffer, "mmap", 4);
			ret = 4;
		} else if (value == WINEFS_SPECIAL_XATTR_SYS_VALUE && size >= 3) {
			memcpy(buffer, "sys", 3);
			ret = 3;
		} else if (value != WINEFS_SPECIAL_XATTR_MMAP_VALUE &&
			   value != WINEFS_SPECIAL_XATTR_SYS_VALUE){
			ret = -ENODATA;
		} else {
			ret = -ERANGE;
		}
	} else {
		if (value == WINEFS_SPECIAL_XATTR_MMAP_VALUE)
			ret = 4;
		else
			ret = 3;
	}
 out:
	inode_unlock(inode);
	return ret;
}
