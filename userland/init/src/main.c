#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

const char *startAppPath = "/usr/bin/bash";

const char *args[] =
{
    "-i", "-l"
};

const char *cwd = "/home/toast/";

void RunApp();

void RunApp()
{
    pid_t pid = fork();

    if(pid == 0)
    {
        int size = sizeof(args) / sizeof(args[0]);

        char **argv = (char **)malloc(sizeof(char *[size + 2]));

        argv[0] = (char *)startAppPath;

        for(int i = 0; i < size; i++)
        {
            argv[i + 1] = (char *)args[i];
        }

        argv[size + 1] = NULL;

        char **envp = (char **)malloc(sizeof(char*[2]));

        envp[0] = "HOME=/home/toast";
        envp[1] = NULL;

        execve(startAppPath, argv, envp);
    }
    else
    {
        int status = 0;

        waitpid(pid, &status, 0);

        RunApp();
    }
}

int main(int argc, char **argv)
{
    RunApp();

    return 0;
}
