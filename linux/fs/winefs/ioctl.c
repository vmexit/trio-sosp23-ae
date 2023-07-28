/*
 * BRIEF DESCRIPTION
 *
 * Ioctl operations.
 *
 * Copyright 2012-2013 Intel Corporation
 * Copyright 2010-2011 Marco Stornelli <marco.stornelli@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/capability.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/compat.h>
#include <linux/mount.h>
#include "winefs.h"
#include "inode.h"

#define	FS_WINEFS_FSYNC	0xBCD0000E

struct sync_range
{
	off_t	offset;
	size_t	length;
};

long winefs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct address_space *mapping = filp->f_mapping;
	struct inode    *inode = mapping->host;
	struct winefs_inode *pi;
	struct super_block *sb = inode->i_sb;
	unsigned int flags;
	int ret;
	winefs_transaction_t *trans;

	pi = winefs_get_inode(sb, inode->i_ino);
	if (!pi)
		return -EACCES;

	switch (cmd) {
	case FS_IOC_GETFLAGS:
		flags = le32_to_cpu(pi->i_flags) & WINEFS_FL_USER_VISIBLE;
		return put_user(flags, (int __user *)arg);
	case FS_IOC_SETFLAGS: {
		unsigned int oldflags;

		ret = mnt_want_write_file(filp);
		if (ret)
			return ret;

		if (!inode_owner_or_capable(&init_user_ns, inode)) {
			ret = -EPERM;
			goto flags_out;
		}

		if (get_user(flags, (int __user *)arg)) {
			ret = -EFAULT;
			goto flags_out;
		}

		inode_lock(inode);
		oldflags = le32_to_cpu(pi->i_flags);

		if ((flags ^ oldflags) &
		    (FS_APPEND_FL | FS_IMMUTABLE_FL)) {
			if (!capable(CAP_LINUX_IMMUTABLE)) {
				inode_unlock(inode);
				ret = -EPERM;
				goto flags_out;
			}
		}

		if (!S_ISDIR(inode->i_mode))
			flags &= ~FS_DIRSYNC_FL;

		flags = flags & FS_FL_USER_MODIFIABLE;
		flags |= oldflags & ~FS_FL_USER_MODIFIABLE;
		inode->i_ctime = current_time(inode);
		trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES, winefs_get_cpuid(sb));
		if (IS_ERR(trans)) {
			ret = PTR_ERR(trans);
			goto out;
		}
		winefs_add_logentry(sb, trans, pi, MAX_DATA_PER_LENTRY, LE_DATA);
		winefs_memunlock_inode(sb, pi);
		pi->i_flags = cpu_to_le32(flags);
		pi->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
		winefs_set_inode_flags(inode, pi);
		winefs_memlock_inode(sb, pi);
		winefs_commit_transaction(sb, trans);
out:
		inode_unlock(inode);
flags_out:
		mnt_drop_write_file(filp);
		return ret;
	}
	case FS_IOC_GETVERSION:
		return put_user(inode->i_generation, (int __user *)arg);
	case FS_IOC_SETVERSION: {
		__u32 generation;
		if (!inode_owner_or_capable(&init_user_ns, inode))
			return -EPERM;
		ret = mnt_want_write_file(filp);
		if (ret)
			return ret;
		if (get_user(generation, (int __user *)arg)) {
			ret = -EFAULT;
			goto setversion_out;
		}
		inode_lock(inode);
		trans = winefs_new_transaction(sb, MAX_INODE_LENTRIES, winefs_get_cpuid(sb));
		if (IS_ERR(trans)) {
			ret = PTR_ERR(trans);
			goto out;
		}
		winefs_add_logentry(sb, trans, pi, sizeof(*pi), LE_DATA);
		inode->i_ctime = current_time(inode);
		inode->i_generation = generation;
		winefs_memunlock_inode(sb, pi);
		pi->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
		pi->i_generation = cpu_to_le32(inode->i_generation);
		winefs_memlock_inode(sb, pi);
		winefs_commit_transaction(sb, trans);
		inode_unlock(inode);
setversion_out:
		mnt_drop_write_file(filp);
		return ret;
	}
	case FS_WINEFS_FSYNC: {
		struct sync_range packet;
		copy_from_user(&packet, (void *)arg, sizeof(struct sync_range));
		winefs_fsync(filp, packet.offset, packet.offset + packet.length, 1);
		return 0;
	}
	case WINEFS_PRINT_TIMING: {
		winefs_print_timing_stats();
		return 0;
	}

	case WINEFS_GET_AVAILABLE_HUGEPAGES: {
		winefs_print_available_hugepages(sb);
		return 0;
	}

	case WINEFS_CLEAR_STATS: {
		winefs_clear_stats();
		return 0;
	}
	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
long winefs_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case FS_IOC32_GETFLAGS:
		cmd = FS_IOC_GETFLAGS;
		break;
	case FS_IOC32_SETFLAGS:
		cmd = FS_IOC_SETFLAGS;
		break;
	case FS_IOC32_GETVERSION:
		cmd = FS_IOC_GETVERSION;
		break;
	case FS_IOC32_SETVERSION:
		cmd = FS_IOC_SETVERSION;
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return winefs_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif
