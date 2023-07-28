#include <linux/string.h>
#include <linux/fs.h>
#include "winefs.h"
#include "xattr.h"

static bool
winefs_xattr_user_list(struct dentry *dentry)
{
	return test_opt(dentry->d_sb, XATTR_USER);
}

static int
winefs_xattr_user_get(const struct xattr_handler *handler,
		    struct dentry *unused, struct inode *inode,
		    const char *name, void *buffer, size_t size)
{
	if (!test_opt(inode->i_sb, XATTR_USER))
		return -EOPNOTSUPP;
	return winefs_xattr_get(inode,
			      name, buffer, size);
}

static int
winefs_xattr_user_set(const struct xattr_handler *handler,
		      struct user_namespace *mnt_userns,
		      struct dentry *unused, struct inode *inode,
		      const char *name, const void *value,
		      size_t size, int flags)
{
	if (!test_opt(inode->i_sb, XATTR_USER))
		return -EOPNOTSUPP;
	return winefs_xattr_set(inode,
				name, value, size, flags);
}

const struct xattr_handler winefs_xattr_user_handler = {
	.prefix	= XATTR_USER_PREFIX,
	.list	= winefs_xattr_user_list,
	.get	= winefs_xattr_user_get,
	.set	= winefs_xattr_user_set,
};
