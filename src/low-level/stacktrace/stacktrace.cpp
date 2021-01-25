#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "debug.hpp"
#include "stacktrace.hpp"

typedef struct stack_frame
{
  struct stack_frame* rbp;
  uint64_t rip;
} stack_frame_t;

static char *symbolData = NULL;
static size_t symbolDataSize = 0;

void kernelInitStacktrace(char *symbols, size_t size)
{
  symbolData = symbols;
  symbolDataSize = size;
}

char* symbolForAddress(uintptr_t* address)
{
    uintptr_t last = 0;
    uintptr_t current = 0;
    char *lastSymbol = 0;
    char *currentSymbol = 0;
    char *curr = (char *)symbolData;

    while (curr < (char *)symbolData + symbolDataSize)
    {
        current = strtol(curr, &currentSymbol, 16);
        currentSymbol = currentSymbol + 1;

        if (!last)
        {
            last = current;
            lastSymbol = currentSymbol;
        }

        if (current > *address)
        {
            *address = last;

            return lastSymbol;
        }

        last = current;
        lastSymbol = currentSymbol;
        curr = strchr(curr, '\n') + 1;
    }

    return NULL;
}

void kernelDumpStacktrace()
{
  DEBUG_OUT("%s", "kernel stacktrace:");

  stack_frame_t* stackframe = NULL;
  __asm__("movq %%rbp, %0" : "=r"(stackframe));

  while (stackframe != NULL) {
    uint64_t address = stackframe->rip;
    char *symbol = symbolForAddress(&address);
    char *end = strchr(symbol, '\n');

    *end = '\0';

    DEBUG_OUT("  %p: %s", address, symbol);

    *end = '\n';

    stackframe = stackframe->rbp;
  }
}
