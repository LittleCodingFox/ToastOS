#pragma once

#include <stdint.h>
#include <stddef.h>

void InitializeLAPIC();
void LAPICEOI();
void LAPICTimerOneShot(uint32_t us, void *function);
void LAPICSENDIPI(uint32_t LAPICID, uint32_t vec);
void LAPICTimerCalibrate();
uint32_t LAPICGetID();
