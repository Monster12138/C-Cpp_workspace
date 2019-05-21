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

#define BUFFER_SIZE 1024

static const char *status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char **argv)
{
    if(argc <= 3)
    {
        printf("usage: %s ip_address port_number filename\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *file_name = argv[3];

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
    int ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if(connfd < 0)
    {
        printf("errno is %d\n", errno);
    }
    else
    {
        char header_buf[BUFFER_SIZE];
        memset(header_buf, '\0', BUFFER_SIZE);
        char *file_buf;
        struct stat file_stat;
        bool valid = true;
        int len = 0;
        if(stat(file_name, &file_stat) < 0)
        {
            valid = false;
        }
        else
        {
            if(S_ISDIR(file_stat.st_mode))
            {
                valid = false;
            }
            else if(file_stat.st_mode & S_IROTH)
            {
                int fd = open(file_name, O_RDONLY);
                file_buf = new char[file_stat.st_size + 1];
                memset(file_buf, '\0', file_stat.st_size + 1);
                if (read(fd, file_buf, file_stat.st_size) < 0)
                {
                    valid = false;
                }
            }
            else
            {
                valid = true;
            }
        }
        if(valid)
        {
            ret = snprintf(header_buf, BUFFER_SIZE-1, "%s %s\r\n",
                           "HTTP/1.1", status_line[0]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE-1-len,
                           "Content-Length: %d\r\n", (int)file_stat.st_size);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE-1-len, "%s", "\r\n");

            struct iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);
            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;
            //int file_size = file_stat.st_size;
            //ret = send(connfd, &file_size, sizeof(file_size), 0);
            //printf("file_size: %d\n", file_size);
            int header_size = strlen(header_buf);
            ret = send(connfd, &header_size, sizeof(header_size), 0);
            printf("header_size: %d\n", header_size);
            int file_name_size = strlen(file_name);
            ret = send(connfd, &file_name_size, sizeof(file_name_size), 0);
            printf("file_name_size: %d\n", ret);
            ret = send(connfd, file_name, strlen(file_name), 0);
            printf("file_name: %s %d\n", file_name, ret);
            ret = writev(connfd, iv, 2);
            printf("http_data: send %d bytes data\n", ret);
        }
        else
        {
            ret = snprintf(header_buf, BUFFER_SIZE-1, "%s %s\r\n",
                           "HTTP/1.1", status_line[1]);
            len += ret;
            ret = snprintf(header_buf, BUFFER_SIZE-1-len, "%s", "\r\n");
            send(connfd, header_buf, strlen(header_buf), 0);
        }
        close(connfd);
        delete []file_buf;
    }
    close(sockfd);
    return 0;
}
