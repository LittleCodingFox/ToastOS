#ifndef KERNEL_H
#define KERNEL_H

#define KPREFIX(n) k_##n

#if __cplusplus
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <frg/string.hpp>
#include <frg/vector.hpp>
#include <frg/manual_box.hpp>
#include <frg_allocator.hpp>

typedef frg::string<frg_allocator> string;

template<typename element>
using vector = frg::vector<element, frg_allocator>;

template<typename element>
using box = frg::manual_box<element>;
#endif

#define ALIGNED(address, align) (!((uint64_t)address & (align - 1)))

#endif
