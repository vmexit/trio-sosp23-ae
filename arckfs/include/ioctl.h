#ifndef SUFS_GLOBAL_IOCTL_H_
#define SUFS_GLOBAL_IOCTL_H_

#define SUFS_CMD_MOUNT         0x1000
#define SUFS_CMD_UMOUNT        0x1001
#define SUFS_CMD_MAP           0x1002
#define SUFS_CMD_UNMAP         0x1003

#define SUFS_CMD_ALLOC_INODE   0x1004
#define SUFS_CMD_FREE_INODE    0x1005

#define SUFS_CMD_GET_PMNODES_INFO  0x1006
#define SUFS_CMD_ALLOC_BLOCK       0x1007
#define SUFS_CMD_FREE_BLOCK        0x1008

#define SUFS_CMD_CHOWN             0x1009
#define SUFS_CMD_CHMOD             0x100a
#define SUFS_CMD_COMMIT            0x100b

#define SUFS_CMD_CHECKER_MAP           0x100c

#define SUFS_CMD_GET_RENAME_LEASE      0x100d
#define SUFS_CMD_FREE_RENAME_LEASE     0x100e

#define SUFS_CMD_DEBUG_READ    0x2000

#define SUFS_CMD_INIT          0x2001
#define SUFS_CMD_SOFT_INIT     0x2002

struct sufs_ioctl_map_entry
{
    char file_type;
    int inode, perm, again;
    unsigned long index_offset;
};


struct sufs_ioctl_inode_alloc_entry
{
    int inode, num, cpu;
};

struct sufs_ioctl_sys_info_entry
{
    int pmnode_num;
    int sockets;
    int cpus_per_socket;
    int dele_ring_per_node;
    void * raddr;
};

struct sufs_ioctl_block_alloc_entry
{
    unsigned long block, num;
    int cpu, pmnode;
};

struct sufs_ioctl_chown_entry
{
    int inode, owner, group;

    unsigned long inode_offset;
};

struct sufs_ioctl_chmod_entry
{
    int inode;
    unsigned int mode;
    unsigned long inode_offset;
};

struct sufs_ioctl_commit_entry
{
    char file_type;
    int inode;
    unsigned long index_offset;
};

struct sufs_ioctl_checker_map_entry
{
    unsigned long max_block;
}; 

struct sufs_ioctl_to_free_entry
{
    int ino_num;
};

struct sufs_ioctl_checker_entry
{
    char file_type; 
    int ino_num, tgid, ret, uid, gid;
    unsigned long index_offset;
};

#endif
