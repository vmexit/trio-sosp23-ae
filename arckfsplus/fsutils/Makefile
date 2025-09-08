CFLAGS = -O2 -lurcu-qsbr

TARGET = cat cp mkdir mv rm ls echof dummy ftruncate init touch mkdir-sufs ls-sufs create create-file write-file webproxy varmail

LDFLAGS = -L../libfs -l:sufs.so -Wl,-R -Wl,../libfs/ -lurcu-qsbr -lpthread

.PHONY: default
default: $(TARGET)

%: %.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)

#do not link with sufs library
init: init.c
	gcc -o init init.c

mkdir-sufs: mkdir.c
	gcc -o mkdir-sufs mkdir.c -lsufs

ls-sufs: ls.c
	gcc -o ls-sufs ls.c -lsufs

.PHONY: install
install:
	sudo cp -a init /usr/bin/init-sufs
	sudo cp -a mkdir-sufs /usr/bin/
	sudo cp -a ls-sufs /usr/bin/
	
.PHONY: clean
clean:
	rm -rf $(TARGET)
