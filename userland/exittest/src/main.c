#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if(argc > 1)
    {
        int code = atoi(argv[1]);

        printf("Exiting with code %i\n", code);

        return code;
    }

    printf("Usage:\n%s [exit code]\n", argv[0]);

    return 0;
}
