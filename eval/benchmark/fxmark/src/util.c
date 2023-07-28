#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <linux/limits.h>


int mkdir_p(const char *path) 
{
    char tmp[PATH_MAX * 2];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s", path);
    len = strlen(tmp);

    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for(p = tmp + 1; *p; p++)
    {
        if(*p == '/') 
        {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(tmp, S_IRWXU);

    return 0;
}

