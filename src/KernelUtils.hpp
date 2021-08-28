#pragma once

#include <stdint.h>
#include <stivale2.h>
#include "framebuffer/FramebufferRenderer.hpp"
#include "cstring/cstring.hpp"
#include "Bitmap.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageMapIndexer.hpp"
#include "paging/Paging.hpp"
#include "paging/PageTableManager.hpp"
#include "text/psf2.hpp"
#include "serial/Serial.hpp"
#include "acpi/ACPI.hpp"
#include "debug.hpp"

struct KernelInfo
{
    PageTableManager* pageTableManager;
};

KernelInfo InitializeKernel(stivale2_struct *stivale2Struct);
uint64_t GetMemorySize(stivale2_struct_tag_memmap *memmap);
