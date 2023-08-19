#ifndef SUFS_GLOBAL_CONFIG_H_
#define SUFS_GLOBAL_CONFIG_H_

/* cache line size */
#define SUFS_CACHELINE 64

#define SUFS_PAGE_SIZE 4096

/* log2(SUFS_PAGE_SIZE) */
#define SUFS_PAGE_SHIFT 12

#define SUFS_PAGE_MASK (~(SUFS_PAGE_SIZE-1))

/* maximum number of CPUs */
#define SUFS_MAX_CPU     224
#define SUFS_MAX_THREADS 448

#define SUFS_MAJOR 269
#define SUFS_DEV_NAME "supremefs"

#define SUFS_DEV_PATH "/dev/"SUFS_DEV_NAME

/* Maximum number of PM device */
#define SUFS_PM_MAX_INS  8

/*
 * Not sure whether we actually need this or not, have it for now to simplify
 * debugging
 */
#define SUFS_MOUNT_ADDR 0x400000000000

#define SUFS_ROOT_INODE 2

/* The maximal number of thread groups that can acquire the lease */
#define SUFS_KFS_LEASE_MAX_OWNER   4
/* What is the length of one lease, in the unit of number of ticks */
#define SUFS_KFS_LEASE_PERIOD    5000

/* maximum number of bytes in the FS */
#define SUFS_KFS_MAX_SIZE (1024ul * 1024 * 1024 * 1024)

/* First inode number that can be accessed by UFS */
#define SUFS_FIRST_UFS_INODE_NUM (3)

/* Maximal number of inode */
#define SUFS_MAX_INODE_NUM    16777216

#define SUFS_SUPER_PAGE_SIZE  4096

/* TODO: Obtain the maximum number of process from sysctl */
#define SUFS_MAX_PROCESS      4194304

/* Ring buffer shared by the uFS and kFS */
#define SUFS_RING_ADDR 0x300000000000

/* Lease ring: whether an inode is in critical section or not */
#define SUFS_LEASE_RING_ADDR (SUFS_RING_ADDR)

#define SUFS_LEASE_RING_SIZE (SUFS_MAX_INODE_NUM / 8)

/* Mapped ring: whether an inode is still being mapped or not */
#define SUFS_MAPPED_RING_ADDR (SUFS_RING_ADDR + SUFS_LEASE_RING_SIZE)

#define SUFS_MAPPED_RING_SIZE (SUFS_MAX_INODE_NUM / 8)

/* counter ring: shared memory to store the complete counter for OdinFS */
#define SUFS_ODIN_CNT_RING_ADDR (SUFS_MAPPED_RING_ADDR + SUFS_MAPPED_RING_SIZE)

/* The actual size is SUFS_CACHELINE * SUFS_PM_MAX_INS */
/* We use a page for each page to allow for NUMA allocation... */
#define SUFS_ODIN_ONE_CNT_RING_SIZE 4096

#define SUFS_ODIN_CNT_RING_TOT_SIZE (SUFS_MAX_THREADS * SUFS_ODIN_ONE_CNT_RING_SIZE)

/* ODIN ring: ring buffer to use OdinFS's access engine in the kernel */
#define SUFS_ODIN_RING_ADDR (SUFS_ODIN_CNT_RING_ADDR + SUFS_ODIN_CNT_RING_TOT_SIZE)

/* Each ring 2MB */
#define SUFS_ODIN_ONE_RING_SIZE (2 * 1024 * 1024)

#define SUFS_ODIN_RING_SIZE (SUFS_ODIN_ONE_RING_SIZE * SUFS_MAX_CPU)

/*
 * Change the type of sufs_kfs_pid_to_tgroup before increasing this value to
 * above 256
 */

#define SUFS_MAX_TGROUP       256

#define SUFS_MAX_PROCESS_PER_TGROUP  32

#define SUFS_NAME_MAX         255

#define SUFS_ROOT_PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | \
                        S_IROTH | S_IWOTH)

#define SUFS_FILE_BLOCK_PAGE_CNT      8
#define SUFS_FILE_BLOCK_SIZE         (SUFS_FILE_BLOCK_PAGE_CNT * SUFS_PAGE_SIZE)
#define SUFS_FILE_BLOCK_SHIFT        15

#define SUFS_DELEGATION_ENABLE 1

/* write delegation limits: 256 */
#define SUFS_WRITE_DELEGATION_LIMIT 256

/* read delegation limits: 32K */
#define SUFS_READ_DELEGATION_LIMIT (32 * 1024)

#define SUFS_CLWB_FLUSH 1

#endif /* SUFS_CONFIG_H_ */
