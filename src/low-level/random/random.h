#pragma once

#include <stdint.h>
#include <stddef.h>

//Based on osdev wiki Standard Example https://wiki.osdev.org/Random_Number_Generator#The_Standard.27s_Example
uint64_t random();
void seedRandom(uint64_t seed);
