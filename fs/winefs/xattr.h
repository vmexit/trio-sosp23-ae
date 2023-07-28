#include <linux/xattr.h>

struct winefs_xattr_info {
	const char *name;
	size_t name_len;
	const void *value;
	size_t value_len;
};

struct winefs_special_xattr_info {
	int name;
	int value;
} __attribute__((packed));

#define WINEFS_SPECIAL_XATTR_NAME 1
#define WINEFS_SPECIAL_XATTR_MMAP_VALUE 1
#define WINEFS_SPECIAL_XATTR_SYS_VALUE 2

extern const struct xattr_handler winefs_xattr_user_handler;
extern const struct xattr_handler *winefs_xattr_handlers[];

int winefs_xattr_set(struct inode *inode, const char *name,
		   const void *value, size_t size,
		   int flags);
ssize_t winefs_listxattr(struct dentry *dentry, char *buffer,
		       size_t buffer_size);
int winefs_xattr_get(struct inode *inode, const char *name,
		   void *buffer, size_t size);
