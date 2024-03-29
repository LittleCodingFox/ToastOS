#pragma once

#include <stdint.h>
#include <stivale2.h>
#include "framebuffer/FramebufferRenderer.hpp"
#include "Bitmap.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "text/psf2.hpp"
#include "serial/Serial.hpp"
#include "acpi/ACPI.hpp"
#include "debug.hpp"

void InitializeKernel(stivale2_struct *stivale2Struct);
uint64_t GetMemorySize(stivale2_struct_tag_memmap *memmap);
