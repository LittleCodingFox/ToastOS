#pragma once

#include "frg/string.hpp"
#include "frg/vector.hpp"
#include "frg_allocator.hpp"

frg::vector<frg::string<frg_allocator>, frg_allocator> SplitString(const frg::string<frg_allocator> &str, char separator);
