#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char buffer[1];

    printf("type keys to print them!\n");

    for(;;)
    {
        size_t count = read(STDIN_FILENO, buffer, 1);
        printf("you typed the character: %c (count: %llu)\n", buffer[0], count);
    }

    return 0;
}
