#pragma once

#include <stdint.h>
#include "elf/elf.hpp"

#define PROCESS_STACK_SIZE 1024

struct ProcessInfo
{
    uint64_t ID;
    char *name;
    char **argv;
    char **environment;
    uint64_t stack[PROCESS_STACK_SIZE];
    uint64_t rsp;
    Elf::ElfHeader *elf;
};

class ProcessManager
{
public:

    ProcessInfo *Execute(const void *image, const char *name, const char **argv);

    ProcessInfo *CurrentProcess();

    void SwitchToUsermode(void *instructionPointer, void *stackPointer);
};

#define PushToStack(stack, value)\
    stack = (char *)stack - sizeof(uint64_t);\
    *((uint64_t *)stack) = (uint64_t)(value);

extern ProcessManager globalProcessManager;
