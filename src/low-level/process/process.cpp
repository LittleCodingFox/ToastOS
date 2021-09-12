#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "paging/PageTableManager.hpp"
#include "process.hpp"

ProcessManager globalProcessManager;

extern "C" void SwitchToUsermode(void *instructionPointer, void *stackPointer);

static ProcessInfo *currentProcess = NULL;

ProcessInfo *ProcessManager::CurrentProcess()
{
    return currentProcess;
}

ProcessInfo *ProcessManager::Execute(const char *name, const char **argv)
{
    if(currentProcess == NULL)
    {
        currentProcess = new ProcessInfo();
        currentProcess->ID = 0;

        for(uint64_t i = 0; i < sizeof(currentProcess->stack); i += 0x1000)
        {
            globalPageTableManager->MapMemory(&currentProcess->stack + i * 0x1000, &currentProcess->stack + i * 0x1000,
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);
        }
    }
    else
    {
        currentProcess->ID++;

        free(currentProcess->name);

        for(uint32_t i = 0; currentProcess->argv[i] != NULL; i++)
        {
            free(currentProcess->argv[i]);
        }

        free(currentProcess->argv);

        for(uint32_t i = 0; currentProcess->environment[i] != NULL; i++)
        {
            free(currentProcess->environment[i]);
        }

        free(currentProcess->environment);
    }

    currentProcess->name = strdup(name);

    memset(currentProcess->stack, 0, sizeof(uint64_t[PROCESS_STACK_SIZE]));

    int argc = 0;

    while(argv[argc])
    {
        argc++;
    }

    char **_argv = (char **)malloc(sizeof(char *[argc + 1]));

    for(uint32_t i = 0; i < argc; i++)
    {
        _argv[i] = strdup(argv[i]);
    }

    _argv[argc] = NULL;

    currentProcess->argv = _argv;

    //TODO: proper env

    char **env = (char **)calloc(1, sizeof(char *[10]));

    currentProcess->environment = env;

    void *stack = (void *)&currentProcess->stack[PROCESS_STACK_SIZE - 1];

    /*
    PushToStack(stack, env);
    PushToStack(stack, _argv);
    PushToStack(stack, argc);
    */

    currentProcess->rsp = (uint64_t)stack;

    return currentProcess;
}

void ProcessManager::SwitchToUsermode(void *instructionPointer, void *stackPointer)
{
    ::SwitchToUsermode(instructionPointer, stackPointer);
}
