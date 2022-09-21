#pragma once

#include <stdint.h>
#include <stddef.h>
#include "interrupts/Interrupts.hpp"

void InitializeLAPIC();
void LAPICEOI();
void LAPICTimerOneShot(uint32_t us, void (*function)(InterruptStack *));
void LAPICSENDIPI(uint32_t LAPICID, uint32_t vec);
void LAPICTimerCalibrate();
uint32_t LAPICGetID();
