#
# Makefile for the linux pmfs-filesystem routines.
#

KDIR ?= ../../linux/


obj-m += winefs.o

winefs-y := bbuild.o balloc.o dir.o file.o inode.o namei.o super.o symlink.o ioctl.o winefs_stats.o journal.o xattr.o xattr_user.o xip.o wprotect.o

all:
	make -C $(KDIR) M=`pwd`

clean:
	make -C $(KDIR) M=`pwd` clean

