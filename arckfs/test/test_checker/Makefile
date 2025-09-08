CFLAGS = -O0 -g -pthread
LDFLAGS = -L../../libfs -l:sufs.so -Wl,-R -Wl,../../libfs/

test_checker: test_checker.c
	gcc $(CFLAGS) -o test_checker test_checker.c $(LDFLAGS)

.PHONY: clean install
clean:
	rm -rf test_checker

install: 
	@echo Do nothing