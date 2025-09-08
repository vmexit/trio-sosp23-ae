CFLAGS = -Ofast -pthread -Wunused -ltcmalloc_minimal
# CFLAGS = -O0 -g -pthread -Iinclude/ -Wunused

TARGET = checker

OBJS = checker.o \
       chainhash.o \
	   simple_ring_buffer.o \
       spinlock.o 


.PHONY: default
default: $(TARGET)

$(TARGET): $(OBJS)
	gcc  $^ -o $@ $(CFLAGS) 
	
%.o: %.c
	gcc -fPIC -c $(CFLAGS) $^

%.o: %.S
	gcc -fPIC -c $(CFLAGS) $^

.PHONY: install
install:
	-kill $(shell pgrep -f checker-sufs)
	sleep 1
	sudo cp -a checker /usr/bin/checker-sufs

.PHONY: clean
clean:
	rm -rf *.o *.so



