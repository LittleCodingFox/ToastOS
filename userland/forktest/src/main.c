#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    pid_t f = fork();

    if(f == 0)
    {
        printf("Client program!\n");
    }
    else
    {
        printf("Forked program!\n");
    }

    return 0;
}
