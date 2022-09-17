#pragma once

#include "interrupts/Interrupts.hpp"

#define PIT_DIVIDEND 1193180

uint16_t PITGetCurrentCount();
void PITSetReloadValue(uint16_t count);
void PITSetFrequency(uint64_t frequency);
void InitializePIT();
