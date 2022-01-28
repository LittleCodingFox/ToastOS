#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    pid_t f = fork();

    if(f == 0)
    {
        printf("Forked program!\n");
    }
    else
    {
        printf("Client program!\n");
    }

    return 0;
}
