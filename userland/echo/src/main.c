#include <stdio.h>

int main(int argc, char **argv)
{
    for(int i = 1; i < argc; i++)
    {
        printf(i > 0 ? " %s" : "%s", argv[i]);
    }

    printf("\n");

    return 0;
}
