#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "libutil.h"

int main(int argc, char *argv[]) 
{
    
    const char *filename = NULL;
    char * buf = NULL;  
    long size = 0;
    int fd = 0, i = 0;

    if (argc != 3) 
    {
        die("Usage: %s <filename> <size_in_bytes>\n", argv[0]);
    }

    filename = argv[1];
    size = strtol(argv[2], NULL, 10);

    if (size <= 0) 
    {
        die("Error: size must be a positive integer\n");
    }

    fd = open(filename, O_WRONLY | O_CREAT, 0644);

    if (fd == -1) 
    {
        die("open error: %s\n", filename);
    }

    buf = malloc(size);
    if (buf == NULL) 
    {
        die("malloc error: %s\n", strerror(errno));
    }
    
    for (i = 0; i < size; i++)
    {
        buf[i] = 'a';  // Fill the buffer with 'a's
    }

    if (write(fd, buf, size) != size)
    {
        die("write error: %s\n", strerror(errno));
    }

    close(fd);

    return 0;
}
