#ifndef KERNEL_H
#define KERNEL_H

#define KPREFIX(n) k_##n

#define USE_INPUT_SYSTEM    1

#ifndef USE_TARFS
#   define USE_TARFS           0
#endif

#include "defines.h"

#if __cplusplus
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <frg/string.hpp>
#include <frg/vector.hpp>
#include <frg/manual_box.hpp>
#include <frg/hash.hpp>
#include <frg/hash_map.hpp>
#include <frg_allocator.hpp>

typedef frg::string<frg_allocator> string;

template<typename element>
using vector = frg::vector<element, frg_allocator>;

template<typename element>
using box = frg::manual_box<element>;

template<typename key, typename value>
using hashmap = frg::hash_map<key, value, frg::hash<key>, frg_allocator>;

#endif

#define ALIGNED(address, align) (!((uint64_t)address & (align - 1)))

#endif
