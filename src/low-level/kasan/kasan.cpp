#include "kernel.h"
#include "Panic.hpp"
#include "paging/PageTableManager.hpp"
#include "debug.hpp"

#if USE_KASAN
namespace {
    constexpr int kasanShift = 3;
    constexpr uintptr_t kasanShadowDelta = 0xdfffe00000000000;
    constexpr bool debugKasan = true;
    constexpr size_t kasanScale = size_t{1} << kasanShift;
    constexpr uintptr_t kasanShadowMemoryBase = kasanShadowDelta + (HIGHER_HALF_MEMORY_OFFSET / kasanScale); //0xFFFFD00000000000
}

extern "C" int8_t *KasanShadowOfPointer(void *ptr)
{
    return (int8_t *)kasanShadowDelta + ((uintptr_t)ptr >> kasanShift);
}

/*
void *KasanPointerOfShadow(int8_t *shadow)
{
    return (void *)(((uintptr_t)shadow - kasanShadowDelta) << kasanShift);
}
*/

bool PointerIsKasanShadow(void *pointer)
{
    return (uint64_t)pointer >= kasanShadowMemoryBase;
}
#endif

extern "C" [[gnu::no_sanitize_address]] void UnpoisonKasanShadow(void *pointer, size_t size)
{
#if USE_KASAN
    if(((uintptr_t)pointer & (kasanScale - 1)) != 0)
    {
        Panic("kasan: invalid pointer at %p", pointer);

        return;
    }

    auto shadow = KasanShadowOfPointer(pointer);

    if(debugKasan)
    {
        DEBUG_OUT("kasan: unpoisoning at %p (%p), size: %lu", pointer, shadow, size);
    }

    for(size_t i = 0; i < (size >> kasanShift); i++)
    {
        if(shadow[i] != (int8_t)0xFF)
        {
            Panic("kasan: invalid shadow %p (%p) at index %i (value: %x)", pointer, shadow, i, shadow[i]);

            return;
        }

        shadow[i] = 0;
    }

    if(size & (kasanScale - 1))
    {
        if(shadow[size >> kasanShift] != (int8_t)0xFF)
        {
            Panic("kasan: invalid shadow %p (%p) at index %i (value: %x)", pointer, shadow, size >> kasanShift, shadow[size >> kasanShift]);

            return;
        }

        shadow[size >> kasanShift] = size & (kasanScale - 1);
    }
#else
    (void)pointer;
    (void)size;
#endif
}

extern "C" [[gnu::no_sanitize_address]] void PoisonKasanShadow(void *pointer, size_t size)
{
#if USE_KASAN
    if(((uintptr_t)pointer & (kasanScale - 1)) != 0)
    {
        Panic("kasan: invalid pointer at %p", pointer);

        return;
    }

    auto shadow = KasanShadowOfPointer(pointer);

    if(debugKasan)
    {
        DEBUG_OUT("kasan: poisoning at %p (%p), size: %lu", pointer, shadow, size);
    }

    for(size_t i = 0; i < (size >> kasanShift); i++)
    {
        if(shadow[i] != (int8_t)0)
        {
            Panic("kasan: invalid shadow %p (%p) at index %i (value: %x)", pointer, shadow, i, shadow[i]);

            return;
        }

        shadow[i] = 0xFF;
    }

    if(size & (kasanScale - 1))
    {
        if(shadow[size >> kasanShift] != (size & (kasanScale - 1)))
        {
            Panic("kasan: invalid shadow %p (%p) at index %i (value: %x)", pointer, shadow, size >> kasanShift, shadow[size >> kasanShift]);

            return;
        }

        shadow[size >> kasanShift] = 0xFF;
    }
#else
    (void)pointer;
    (void)size;
#endif
}

[[gnu::no_sanitize_address]] void CleanKasanShadow(void *pointer, size_t size)
{
#if USE_KASAN
    if(((uintptr_t)pointer & (kasanScale - 1)) != 0)
    {
        Panic("kasan: invalid pointer at %p", pointer);

        return;
    }

    if(debugKasan)
    {
        DEBUG_OUT("kasan: Cleaning at %p, size: %lu", pointer, size);
    }

    auto shadow = KasanShadowOfPointer(pointer);

    for(size_t i = 0; i < (size >> kasanShift); i++)
    {
        shadow[i] = 0;
    }

    if(size & (kasanScale - 1))
    {
        shadow[size >> kasanShift] = size & (kasanScale - 1);
    }
#else
    (void)pointer;
    (void)size;
#endif
}

[[gnu::no_sanitize_address]] void ValidateKasanClean(void *pointer, size_t size)
{
#if USE_KASAN
    if(((uintptr_t)pointer & (kasanScale - 1)) != 0)
    {
        Panic("kasan: invalid pointer at %p", pointer);
    }

    auto shadow = KasanShadowOfPointer(pointer);

    for(size_t i = 0; i < (size >> kasanShift); i++)
    {
        if(shadow[i] != 0)
        {
            Panic("kasan: invalid kasan clean at %p (%p) (size: %lu)", pointer, shadow, size);

            return;
        }
    }
#else
    (void)pointer;
    (void)size;
#endif
}

#if USE_KASAN
extern "C" void __asan_alloca_poison(uintptr_t address, size_t size)
{
    (void)address;
    (void)size;
}

extern "C" void __asan_allocas_unpoison(uintptr_t address, size_t size)
{
    (void)address;
    (void)size;
}

extern "C" [[gnu::no_sanitize_address]]
void __asan_set_shadow_00(void *pointer, size_t size)
{
    auto p = (int8_t *)pointer;

    for(size_t i = 0; i < size; i++)
    {
        p[i] = 0;
    }
}

namespace {

    [[gnu::no_sanitize_address]]
    void Report(bool write, uintptr_t address, size_t size, void *ip)
    {
        Panic("kasan failure at IP %p, size %lu-byte, %s address %p",
            ip, size, (write ? "write to" : "read from"), address);
    }
}

extern "C" void __asan_report_load1_noabort(uintptr_t address)
{
    Report(false, address, 1, __builtin_return_address(0));
}

extern "C" void __asan_report_load2_noabort(uintptr_t address)
{
    Report(false, address, 2, __builtin_return_address(0));
}

extern "C" void __asan_report_load4_noabort(uintptr_t address)
{
    Report(false, address, 4, __builtin_return_address(0));
}

extern "C" void __asan_report_load8_noabort(uintptr_t address)
{
    Report(false, address, 8, __builtin_return_address(0));
}

extern "C" void __asan_report_load16_noabort(uintptr_t address)
{
    Report(false, address, 16, __builtin_return_address(0));
}

extern "C" void __asan_report_load_n_noabort(uintptr_t address, size_t size)
{
    Report(false, address, size, __builtin_return_address(0));
}

extern "C" void __asan_report_store1_noabort(uintptr_t address)
{
    Report(true, address, 1, __builtin_return_address(0));
}

extern "C" void __asan_report_store2_noabort(uintptr_t address)
{
    Report(true, address, 2, __builtin_return_address(0));
}

extern "C" void __asan_report_store4_noabort(uintptr_t address)
{
    Report(true, address, 4, __builtin_return_address(0));
}

extern "C" void __asan_report_store8_noabort(uintptr_t address)
{
    Report(true, address, 8, __builtin_return_address(0));
}

extern "C" void __asan_report_store16_noabort(uintptr_t address)
{
    Report(true, address, 16, __builtin_return_address(0));
}

extern "C" void __asan_report_store_n_noabort(uintptr_t address, size_t size)
{
    Report(true, address, size, __builtin_return_address(0));
}

extern "C" void __asan_handle_no_return()
{
}

#endif
