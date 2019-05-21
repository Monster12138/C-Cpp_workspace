#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char **argv)
{
    if(argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    //address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
    int opt;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);

    //recv(sockfd, &file_size, sizeof(file_size), 0);
    //printf("file_size: %d\n", file_size);

    int header_size;
    recv(sockfd, &header_size, sizeof(header_size), 0);
    printf("header_size: %d\n", header_size);

    int file_name_size;
    recv(sockfd, &file_name_size, sizeof(file_name_size), 0);
    printf("file_name_size: %d\n", file_name_size);

    char file_name[1024] = {0};
    ret = recv(sockfd, file_name, file_name_size, 0);
    printf("file_name: %s\n\n", file_name);

    char *header_buf = new char[header_size + 1];
    memset(header_buf, '\0', header_size + 1);
    ret = recv(sockfd, header_buf, header_size, 0);
    printf("%s\n", header_buf);

    int file_size;
    char *ptr = strstr(header_buf, "Content-Length: ");
    ptr += sizeof("Content-Length: ") - 1;
    file_size = atoi(ptr);
    char *file_buf = new char[file_size + 1];
    memset(file_buf, '\0', file_size + 1);
    ret = recv(sockfd, file_buf, file_size, 0);
    printf("%s\n", file_buf);
    close(sockfd);

    int fd = open(file_name, O_RDWR);
    ret = write(fd, file_buf, file_size);
    printf("write %d bytes data to %s\n", ret, file_name);
    close(fd);
 
    return 0;
}

