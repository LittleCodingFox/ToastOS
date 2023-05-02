#include "ubsan.hpp"
#include "debug.hpp"
#include "Panic.hpp"

void ubsanPanicAt(ubsan_source_location_t *location, const char *error)
{
    if(location)
    {
        DEBUG_OUT("%s:%d:%d -> %s", location->file, location->line, location->column, error);
    }

    Panic("UBSan Exception: %s", error);
}

void ubsanPanic(const char *error)
{
    DEBUG_OUT("[UBSAN] %s", error);

    Panic("UBSan Exception: %s", error);
}

extern "C" void __ubsan_handle_type_mismatch_v1(ubsan_mismatch_data_t *data, uintptr_t ptr)
{
    if(ptr == 0)
    {
        ubsanPanicAt(&data->location, "NULL pointer access");
    }
    else if(data->align != 0 && (ptr & ((1 << data->align) - 1)) != 0)
    {
        DEBUG_OUT("[UBSAN] pointer %p not aligned to %d", (void *)ptr, 1 << data->align);
        
        ubsanPanicAt(&data->location, "Pointer alignment failed");
    }
    else
    {
        DEBUG_OUT("[UBSAN] Pointer %p is not large enough for %s", (void *)ptr, data->type->name);
    }
}

extern "C" void __ubsan_handle_add_overflow()
{
    ubsanPanic("add overflow");
}

extern "C" void __ubsan_handle_sub_overflow()
{
    ubsanPanic("sub overflow");
}

extern "C" void __ubsan_handle_mul_overflow(void *d, void *l, void *r)
{
    ubsan_overflow_data_t *data = (ubsan_overflow_data_t *)d;

    DEBUG_OUT("[UBSAN] overflow in %ld*%ld", (int64_t)l, (int64_t)r);

    ubsanPanicAt(&data->location, "mul overflow");
}

extern "C" void __ubsan_handle_divrem_overflow()
{
    ubsanPanic("divrem overflow");
}

extern "C" void __ubsan_handle_negate_overflow()
{
    ubsanPanic("negate overflow");
}

extern "C" void __ubsan_handle_pointer_overflow(void* data_raw, void* lhs_raw, void* rhs_raw)
{
    ubsan_overflow_data_t* data = (ubsan_overflow_data_t*) data_raw;

    DEBUG_OUT("[UBSAN] pointer overflow with operands %p, %p", lhs_raw, rhs_raw);

    ubsanPanicAt(&data->location, "pointer overflow");
}

extern "C" void __ubsan_handle_out_of_bounds(void* data, void* index)
{
    ubsan_out_of_bounds_data_t* d = (ubsan_out_of_bounds_data_t*) data;

    DEBUG_OUT("[UBSAN] out of bounds at index %ld", (uint64_t) index);

    ubsanPanicAt(&d->location, "out of bounds");
}

extern "C" void __ubsan_handle_shift_out_of_bounds(void* data, void* lhs, void* rhs)
{
    ubsan_shift_out_of_bounds_data_t* d = (ubsan_shift_out_of_bounds_data_t*) data;

    DEBUG_OUT("[UBSAN] %ld << %ld", (uint64_t) lhs, (uint64_t) rhs);

    ubsanPanicAt(&d->location, "shift out of bounds");
}

extern "C" void __ubsan_handle_load_invalid_value()
{
    ubsanPanic("load invalid value");
}

extern "C" void __ubsan_handle_float_cast_overflow()
{
    ubsanPanic("float cast overflow");
}

extern "C" void __ubsan_handle_builtin_unreachable()
{
    ubsanPanic("builtin unreachable");
}

extern "C" void __ubsan_handle_missing_return(ubsan_source_location_t *location)
{
    ubsanPanicAt(location, "Missing return");
}

extern "C" void __ubsan_handle_vla_bound_not_positive(ubsan_overflow_data_t *data, uintptr_t ptr)
{
    ubsanPanicAt(&data->location, "Negative VLA size");
}

extern "C" void __ubsan_handle_alignment_assumption(void *data, uintptr_t pointer, uint32_t align, uint32_t offset)
{
    ubsan_alignment_assumption_data_t *alignmentData = (ubsan_alignment_assumption_data_t *)data;
    uintptr_t realPtr;

    if(offset)
    {
        DEBUG_OUT("[UBSAN] assumption of %u byte alignment (with offset %u byte) for pointer of type %s failed", align, offset, alignmentData->type->name);
    }
    else
    {
        DEBUG_OUT("[UBSAN] assumption of %u byte alignment for pointer of type %s failed", align, alignmentData->type->name);
    }

    realPtr = pointer - offset;

    DEBUG_OUT("%s address misalignment offset is %lu bytes",
        offset ? "offset" : "", realPtr & (align - 1));

    ubsanPanicAt(&alignmentData->assumption_location, "byte alignment failed");
}