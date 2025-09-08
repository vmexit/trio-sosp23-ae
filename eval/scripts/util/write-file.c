#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define CHUNK_SIZE 4096

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
    }

    filename = argv[1];
    ch_str = argv[2];

    if (ch_str[0] == '\0' || ch_str[1] != '\0') 
    {
    }

    repeat = strtol(argv[3], NULL, 10);

    if (repeat <= 0) 
    {
    }

    old_size = strtol(argv[4], NULL, 10);

    offset = strtol(argv[5], NULL, 10);

    pos = offset;

    ch = ch_str[0];

    memset(buf, ch, CHUNK_SIZE);

    fd = open(filename, O_WRONLY);
    if (fd == -1) 
    {
    }

    for (i = 0; i < repeat; i++) 
    {
        written = pwrite(fd, buf, CHUNK_SIZE, pos);
        if (written != CHUNK_SIZE) 
        {
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
