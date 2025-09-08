CFLAGS = -O0 -g 
LDFLAGS = -L../../libfs -l:sufs.so -Wl,-R -Wl,../../libfs/

test_remove: test_remove.c
	gcc $(CFLAGS) -o test_remove test_remove.c $(LDFLAGS)

.PHONY: run
run: test_remove
	./test_remove /sufs/test/

.PHONY: clean
clean:
	rm -rf test_remove
