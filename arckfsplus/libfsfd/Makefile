# https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html

#ASAN_OPTIONS=verify_asan_link_order=0
#CFLAGS = -O0 -g -pthread -Iinclude/ -Wunused -fsanitize=address
#CFLAGS = -O0 -g -pthread -Iinclude/ -Wunused
CFLAGS = -Ofast -pthread -Iinclude/ -Wunused -ltcmalloc_minimal

#LDFLAGS = -shared -pthread -O0 -g -ldl -lnuma -fsanitize=address
#LDFLAGS = -shared -pthread -O0 -g -ldl -lnuma
LDFLAGS = -shared -pthread -Ofast -ldl -lnuma -ltcmalloc_minimal

TARGET=sufsfd.so

OBJS = balloc.o \
       chainhash.o \
       cmd.o \
       delegation.o \
       dir.o \
       entry.o \
       file.o \
       filetable.o \
       ialloc.o \
       journal.o \
       mfs.o \
       mnode.o \
       proc.o \
       radix_array.o \
       range_lock.o \
       rb.o \
       rbtree.o \
       rwlock_bravo.o \
       simple_ring_buffer.o \
       spinlock.o \
       stat.o  \
       super.o \
       syscall.o \
       tls.o \
       util.o

.PHONY: default
default: $(TARGET)

$(TARGET): $(OBJS)
	gcc  $^ -o $@ $(CFLAGS) $(LDFLAGS)
	
%.o: %.c
	gcc -fPIC -c $(CFLAGS) $^

%.o: %.S
	gcc -fPIC -c $(CFLAGS) $^

.PHONY: install
install:
	sudo cp sufsfd.so /lib/x86_64-linux-gnu/libsufsfd.so
	
.PHONY: clean
clean:
	rm -rf *.o *.so