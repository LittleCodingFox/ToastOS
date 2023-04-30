#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int fd;
char buff[8192];
pthread_t thread;

void *ServerThread(void *)
{
    int clientFD = accept(fd, NULL, NULL);

    printf("Client FD: %i\n", clientFD);

    int length;

    while((length = recv(clientFD, buff, sizeof(buff), 0) > 0))
    {
        printf("Server received: %s\n", buff);

        strcpy(buff, "SOK");

        int result = send(clientFD, buff, strlen(buff) + 1, 0);

        if(result < 0)
        {
            printf("Server Error: send\n");

            return NULL;
        }
    }

    printf("Server Exit\n");

    return NULL;
}

int main(int argc, char **argv)
{
    if((fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error: socket\n");

        return 1;
    }

    if(argc > 1 && strcmp(argv[1], "--client") == 0)
    {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "server.sock");

        if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            printf("Client: Failed to connect\n");

            return 1;
        }

        for(;;)
        {
            printf("Input Client Message: ");

            scanf("%s", buff);

            send(fd, buff, strlen(buff) + 1, 0);

            int length = recv(fd, buff, sizeof(buff), 0);

            if(length > 0)
            {
                printf("Client received: %s\n", buff);
            }
            else
            {
                printf("Client Error\n");
            }
        }

        printf("Client Exit!\n");
    }
    else
    {
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

        if(listen(fd, 1000) < 0)
        {
            printf("Error: listen\n");

            return 1;
        }

        printf("Server started\n");

        pthread_create(&thread, NULL, ServerThread, NULL);

        pthread_join(thread, NULL);
    }

    return 0;
}
