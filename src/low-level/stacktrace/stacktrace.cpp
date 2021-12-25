#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "debug.hpp"
#include "stacktrace.hpp"
#include "paging/Paging.hpp"

typedef struct stack_frame
{
  struct stack_frame* rbp;
  uint64_t rip;
} stack_frame_t;

static char *symbolData = NULL;
static size_t symbolDataSize = 0;
static bool kernelStacktraceAvailable = false;

bool KernelStacktraceAvailable()
{
  return kernelStacktraceAvailable;
}

void KernelInitStacktrace(char *symbols, size_t size)
{
  symbolData = symbols;
  symbolDataSize = size;

  kernelStacktraceAvailable = true;
}

char* SymbolForAddress(uintptr_t* address)
{
  uintptr_t last = 0;
  uintptr_t current = 0;
  char *lastSymbol = 0;
  char *currentSymbol = 0;
  char *curr = (char *)symbolData;

  while (curr < (char *)symbolData + symbolDataSize)
  {
      current = strtoull(curr, &currentSymbol, 16);

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

void KernelDumpStacktrace()
{
  DEBUG_OUT("%s", "kernel stacktrace:");

  stack_frame_t* stackframe = NULL;

  __asm__("movq %%rbp, %0" : "=r"(stackframe));

  while (stackframe != NULL)
  {
    uint64_t address = stackframe->rip;

    /*
    char *symbol = SymbolForAddress(&address);
    char *end = strchr(symbol, '\n');

    *end = '\0';

    DEBUG_OUT("  %p (%p): %s", address, stackframe->rip, symbol);
    */

    DEBUG_OUT("  %p", address);

    //*end = '\n';

    stackframe = stackframe->rbp;
  }

  DEBUG_OUT("%s", "stacktrace end");
}

void KernelDumpStacktraceNoLock()
{
  DEBUG_OUT_NOLOCK("%s", "kernel stacktrace:");

  stack_frame_t* stackframe = NULL;

  __asm__("movq %%rbp, %0" : "=r"(stackframe));

  while (stackframe != NULL)
  {
    uint64_t address = stackframe->rip;

    /*
    char *symbol = SymbolForAddress(&address);
    char *end = strchr(symbol, '\n');

    *end = '\0';

    DEBUG_OUT_NOLOCK("  %p (%p): %s", address, stackframe->rip, symbol);
    */

    DEBUG_OUT_NOLOCK("  %p", address);

    //*end = '\n';

    stackframe = stackframe->rbp;
  }

  DEBUG_OUT_NOLOCK("%s", "stacktrace end");
}
