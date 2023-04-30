#pragma once

#include <stdlib.h>

struct frg_allocator
{
    void *allocate(size_t s)
    {
        return malloc(s);
    }

    void deallocate(void *p, size_t)
    {
        if(p)
        {
            ::free(p);
        }
    }

    void free(void *p)
    {
        deallocate(p, 0);
    }

    static frg_allocator &get()
    {
        static frg_allocator alloc;

        return alloc;
    }
};
