#include <stdio.h>

int main(int argc, char **argv)
{
    char buffer[1];

    printf("type keys to print them!\n");

    for(;;)
    {
        size_t count = fread(buffer, 1, 1, stdin);
        
        printf("you typed the character: %c (count: %llu)\n", buffer[0], count);
    }

    return 0;
}
