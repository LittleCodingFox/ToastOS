#pragma once

#include <stdint.h>

class FPU
{
public:
    void initialize();
    void kernelEnter();
    void kernelExit();
};

extern FPU fpu;
