KDIR ?= ../../linux/

obj-m += sufs.o

sufs-y := agent.o balloc.o checker.o delegation.o dev_dax.o device.o inode.o lease.o \
	mmap.o rbtree.o ring_buffer.o ring.o simple_ring_buffer.o spinlock.o super.o \
	tgroup.o
             
ccflags-y := -std=gnu99 -Wno-declaration-after-statement -O3 -mtune=native

.PHONY: all
all:
	make -C $(KDIR) M=`pwd`

.PHONY: clean
clean:
	make -C $(KDIR) M=`pwd` clean
	
.PHONY: install
install:
	sudo mkdir -p /usr/lib/modules/$(shell uname -r)/
	sudo cp -a sufs.ko /usr/lib/modules/$(shell uname -r)/
