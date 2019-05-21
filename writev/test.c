#include <sys/stat.h>
#include <stdio.h>


int main()
{
    struct stat buf;
    stat("server.cc", &buf);
    printf("file size: %d\n", buf.st_size);
    return 0;
}

