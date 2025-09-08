CFLAGS = -O0 -g 
LDFLAGS = -L../../libfs -l:sufs.so -Wl,-R -Wl,../../libfs/

test_rw: test_rw.c
	gcc $(CFLAGS) -o test_rw test_rw.c $(LDFLAGS)

.PHONY: run
run: test_rw
	./test_rw 2000000 1 10485760 5000 50

.PHONY: clean
clean:
	rm -rf test_rw
