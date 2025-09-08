CFLAGS = -O0 -g 
LDFLAGS = -L../../libfs -l:sufs.so -Wl,-R -Wl,../../libfs/

test_rename: test_rename.c
	gcc $(CFLAGS) -o test_rename test_rename.c $(LDFLAGS)

.PHONY: run
run: test_rename
	./test_rename /sufs/test/ /sufs/test2/

.PHONY: clean
clean:
	rm -rf test_rename
