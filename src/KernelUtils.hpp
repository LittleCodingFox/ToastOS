#pragma once

#include <stdint.h>
#include <limine.h>
#include "framebuffer/FramebufferRenderer.hpp"
#include "Bitmap.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "text/psf2.hpp"
#include "serial/Serial.hpp"
#include "acpi/ACPI.hpp"
#include "debug.hpp"

void InitializeKernel(volatile limine_framebuffer_request *framebuffer, volatile limine_memmap_request *memmap, volatile limine_module_request *modules,
    volatile limine_hhdm_request *hhdm, volatile limine_kernel_address_request *kernelAddress);
uint64_t GetMemorySize(volatile limine_memmap_request *memmap);
