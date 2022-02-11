#include <stdio.h>
#include <toast/syscall.h>

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("Usage: %s [layout name]\n", argv[0]);

        return 1;
    }

    int result = syscall(SYSCALL_SETKBLAYOUT, argv[1]);

    printf(result == 1 ? "Successfully loaded keyboard layout %s\n" :
        "Failed to load keyboard layout %s\n", argv[1]);

    return result == 1 ? 0 : 1;
}
