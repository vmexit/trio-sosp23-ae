#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "libutil.h"

#define CHUNK_SIZE 4096

struct thread_arg {
    const char *filename;
    const char *ch_str;
    long repeat;
    long old_size;
    long offset;
};

static void *writer_thread(void *arg) {
    struct thread_arg *t = (struct thread_arg *)arg;

    const char *filename = NULL; 
    const char *ch_str = NULL; 
    long repeat = 0, old_size = 0;
    char ch = 0; 
    char buf[CHUNK_SIZE];
    int fd = 0, i = 0; 
    int written = 0, pos = 0, offset = 0;

    filename = t->filename;
    ch_str = t->ch_str;
    repeat = t->repeat;
    old_size = t->old_size;
    offset = t->offset;

    pos = offset;

    ch = ch_str[0];

    memset(buf, ch, CHUNK_SIZE);

    fd = open(filename, O_WRONLY);
    if (fd == -1) 
    {
        die("open error: %s\n", filename);
    }

    for (i = 0; i < repeat; i++) 
    {
        written = pwrite(fd, buf, CHUNK_SIZE, pos);
        if (written != CHUNK_SIZE) 
        {
            die("write");
        }

        pos += CHUNK_SIZE * 2;

        if (pos >= old_size)
        {
            pos = offset;   
        }
    }
    close(fd);

    printf("I am done!\n");

    return 0;
}

int main(int argc, char *argv[]) 
{
    const char *filename = NULL; 
    const char *ch_str = NULL; 
    long repeat = 0, old_size = 0;
    char ch = 0; 
    char buf[CHUNK_SIZE];
    int fd = 0, i = 0; 
    int written = 0, pos = 0, offset = 0;

    if (argc != 6) 
    {
        die("Usage: %s <filename> <char> <repeat_count> <old_size> <offset>\n", 
            argv[0]);
    }

    filename = argv[1];
    ch_str = argv[2];

    if (ch_str[0] == '\0' || ch_str[1] != '\0') 
    {
        die("Error: <char> must be a single character.\n");
    }

    repeat = strtol(argv[3], NULL, 10);

    if (repeat <= 0) 
    {
        die("Error: <repeat_count> must be positive.\n");
    }

    old_size = strtol(argv[4], NULL, 10);

    offset = strtol(argv[5], NULL, 10);

    pthread_t t1, t2;
    struct thread_arg a1 = {
        .filename = filename,
        .ch_str = ch_str,
        .repeat = repeat,
        .old_size = old_size,
        .offset = offset,
    };
    
    struct thread_arg a2 = a1;
    a2.offset = 4096;
    a2.ch_str = "b";

    int rc;
    rc = pthread_create(&t1, NULL, writer_thread, &a1);
    if (rc) die("pthread_create t1: %s\n", strerror(rc));
    rc = pthread_create(&t2, NULL, writer_thread, &a2);
    if (rc) die("pthread_create t2: %s\n", strerror(rc));

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
