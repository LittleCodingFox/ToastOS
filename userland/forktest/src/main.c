#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    pid_t f = fork();

    if(f == 0)
    {
        printf("Forked program with pid %i!\n", getpid());
    }
    else
    {
        printf("Client program with pid %i and child pid %i!\n", getpid(), f);

        int status;
        pid_t out = waitpid(-1, &status, 0);

        printf("fork ended with pid %i and status %x", out, status);
    }

    return 0;
}
