CFLAGS = -O0 -g 
LDFLAGS = -L../../libfs -l:sufs.so -Wl,-R -Wl,../../libfs/

test_append: test_append.c
	gcc $(CFLAGS) -o test_append test_append.c $(LDFLAGS)

.PHONY: run
run: test_append
	./test_append 4194304

.PHONY: clean
clean:
	rm -rf test_append
