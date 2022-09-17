#pragma once

#include <stdint.h>
#include <stddef.h>

void IOAPICSetIRQRedirect(uint32_t LAPICID, uint8_t vector, uint8_t irq, bool status);
void IOAPICSetGSIRedirect(uint32_t LAPICID, uint8_t vector, uint8_t gsi, uint16_t flags, bool status);
