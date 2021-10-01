#include <stdio.h>

void _start()
{
    char buffer[1];

    printf("type keys to print them!\n");

    for(;;)
    {
        read(stdin, buffer, 1);

        printf("you typed the character: %c\n", buffer[0]);
    }
}
