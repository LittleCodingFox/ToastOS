#include <stdio.h>
#include <fcntl.h>

void _start()
{
    char buffer[1];

    printf("type keys to print them!\n");

    for(;;)
    {
        size_t count = read(stdin, buffer, 1);
        printf("you typed the character: %c (count: %llu)\n", buffer[0], count);
    }
}
