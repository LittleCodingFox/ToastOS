#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if(argc > 1 && strcmp(argv[1], "--client") == 0)
    {
        printf("client\n");
    }
    else
    {
        char buff[8192];

        int fd;

        if((fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0)
        {
            printf("Error: socket\n");

            return 1;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "server.sock");
        unlink("server.sock");

        if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            printf("Error: bind\n");

            return 1;
        }

        struct sockaddr_un from;
        socklen_t fromLength = sizeof(from);
        int length;

        while((length = recvfrom(fd, buff, sizeof(buff), 0, (struct sockaddr *)&from, &fromLength)) > 0)
        {
            printf("received: %s\n", buff);

            strcpy(buff, "OK");

            int result = sendto(fd, buff, strlen(buff) + 1, 0, (struct sockaddr *)&from, fromLength);

            if(result < 0)
            {
                printf("Error: sendto\n");

                return 1;
            }
        }
    }

    return 0;
}
