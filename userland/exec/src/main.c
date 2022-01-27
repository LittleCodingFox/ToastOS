#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if(argc > 1)
    {
        const char *path = argv[1];

        char **args = (char **)malloc(sizeof(char *[argc - 1]));
        char **envp = (char **)malloc(sizeof(char *[1]));

        envp[0] = NULL;

        for(int i = 2; i < argc; i++)
        {
            args[i - 2] = argv[i];            
        }

        args[argc - 2] = NULL;
       
        execve(path, args, envp);

        return 1;
    }

    return 0;
}
