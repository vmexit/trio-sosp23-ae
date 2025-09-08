CFLAGS = -O0 -g

TARGET = create write-file create-file-tg

LDFLAGS = -lpthread

.PHONY: default
default: $(TARGET)

%: %.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
.PHONY: clean
clean:
	rm -rf $(TARGET)
