#pragma once

#include <stdint.h>
#include "framebuffer/FramebufferRenderer.hpp"
#include "cstring/cstring.hpp"
#include "efimemory/EFIMemory.hpp"
#include "Bitmap.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageMapIndexer.hpp"
#include "paging/Paging.hpp"
#include "paging/PageTableManager.hpp"
#include "text/psf2.hpp"
#include "serial/Serial.hpp"
#include "acpi/ACPI.hpp"
#include "debug.hpp"

struct BootInfo
{
	Framebuffer* framebuffer;
	psf2_font_t* font;
	char *symbols;
	size_t symbolsSize;
	EFI_MEMORY_DESCRIPTOR* mMap;
	uint64_t mMapSize;
	uint64_t mMapDescSize;
	RSDP2 *rsdp;
	SDTHeader *xsdt;
};

extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;

struct KernelInfo
{
    PageTableManager* pageTableManager;
};

KernelInfo InitializeKernel(BootInfo* BootInfo);
