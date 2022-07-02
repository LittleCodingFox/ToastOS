#pragma once
#include <stddef.h>
#include <stdint.h>

extern "C" void UnpoisonKasanShadow(void *pointer, size_t size);
extern "C" void PoisonKasanShadow(void *pointer, size_t size);
bool PointerIsKasanShadow(void *pointer);