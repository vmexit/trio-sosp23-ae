main: main.c
	gcc -g -o main main.c
	
.PHONY: run
run:
	LD_PRELOAD=../../libfs/sufs.so ./main
	
.PHONY: clean
clean:
	rm -rf main