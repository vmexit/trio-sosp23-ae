#ifndef SUFS_CHECKER_LIBFS_CALL_H_
#define SUFS_CHECKER_LIBFS_CALL_H_

void * sufs_libfs_getinode(int fd);
void * sufs_libfs_getdentry(int fd);
void * sufs_libfs_get_dentry_by_path(char * path);
void *sufs_libfs_get_inode_by_path(char *path);
int sufs_libfs_commit(int fd);
int sufs_libfs_commit_by_path(char *path);
int sufs_unmap_by_path(char * path);


#endif