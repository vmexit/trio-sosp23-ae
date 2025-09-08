CFLAGS = -O0 -g -pthread
LDFLAGS = -L../../libfs -l:sufs.so -Wl,-R -Wl,../../libfs/

test_create: test_create.c
	gcc $(CFLAGS) -o test_create test_create.c $(LDFLAGS)

.PHONY: run
run: test_create
	./test_create 1 1

.PHONY: clean
clean:
	rm -rf test_create
