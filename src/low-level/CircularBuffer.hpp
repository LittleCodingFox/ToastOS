#pragma once

#include "kernel.h"
#include "threading/lock.hpp"

template<typename Element>
class CircularBuffer
{
private:
    AtomicLock lock;
    vector<Element> buffer;
    size_t head;
    size_t tail;
    bool full;
public:
    explicit CircularBuffer(size_t size) : head(0), tail(0), full(false)
    {
        buffer.resize(size);
    }

    void Put(Element element)
    {
        ScopedLock lock(this->lock);

        buffer[head] = element;

        if(full)
        {
            if(++tail == buffer.size())
            {
                tail = 0;
            }
        }

        if(++head == buffer.size())
        {
            head = 0;
        }

        full = head == tail;
    }

    Element *Get()
    {
        ScopedLock lock(this->lock);

        if(Empty())
        {
            return NULL;
        }

        Element *value = &buffer[tail];

        full = false;

        if(++tail == buffer.size())
        {
            tail = 0;
        }

        return value;
    }

    void Reset()
    {
        ScopedLock lock(this->lock);

        head = tail;
        full = false;
    }

    inline bool Empty() const
    {
        return !full && head == tail;
    }

    inline bool Full() const
    {
        return full;
    }

    size_t Capacity() const
    {
        return buffer.size();
    }

    size_t Size() const
    {
        if(!full)
        {
            if(head > tail)
            {
                return head - tail;
            }

            return buffer.size() + head - tail;
        }

        return buffer.size();
    }
};
