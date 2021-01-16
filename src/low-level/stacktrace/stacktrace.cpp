#include <stdint.h>
#include "debug.hpp"
#include "stacktrace.hpp"

typedef struct stack_frame
{
  struct stack_frame* rbp;
  uint64_t rip;
} stack_frame_t;

void kernel_dump_stacktrace()
{
  DEBUG_OUT("%s", "kernel stacktrace:");

  stack_frame_t* stackframe = NULL;
  __asm__("movq %%rbp, %0" : "=r"(stackframe));

  while (stackframe != NULL) {
    uint64_t address = stackframe->rip;

    DEBUG_OUT("  %p", address);

    stackframe = stackframe->rbp;
  }
}
