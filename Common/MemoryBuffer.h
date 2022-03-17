#pragma once

//////////////////////////////////////////////////////////////////////
// ultra basic expanding buffer for storing error response data and memory downloads
// TODO (chs): compare code size of this with std::vector, replace if std::vector is smaller

template <typename T> struct MemoryBuffer
{
    size_t capacity = 0;
    size_t size = 0;
    T *data = null;

    MemoryBuffer() = default;

    ~MemoryBuffer()
    {
        clear();
    }

    void add(T const *d, size_t count)
    {
        add_capacity(count);
        for(size_t i = 0; i < count; ++i) { // memcpy trait later
            data[i + size] = d[i];
        }
        size += count;
    }

    void add(T const &d)
    {
        add(&d, 1);
    }

    T &operator[](size_t index)
    {
        return data[index];
    }

    void set_length(size_t s)
    {
        if(s > size) {
            add_capacity(s - size);
        }
        size = s;
    }

    void trim_to_fit()
    {
        if(capacity > size) {
            if(size > 0) {
                capacity = size;
                move_to_new();
            }
            else {
                delete[] data;
                data = null;
                capacity = 0;
            }
        }
    }

    void clear()
    {
        delete[] data;
        data = null;
        capacity = 0;
        size = 0;
    }

    void set_capacity(size_t new_capacity)
    {
        if(new_capacity > capacity) {
            add_capacity(new_capacity - capacity);
        }
        else if(new_capacity < capacity) {
            capacity = new_capacity;
            move_to_new();
        }
    }

    void add_capacity(size_t amount)
    {
        if(amount == 0) {
            return;
        }
        size_t old_capacity = capacity;
        if(capacity == 0) {
            capacity = 8;
        }
        while((size + amount) > capacity) {
            capacity *= 2; // fugit, just double it for now, add a trait later
        }
        if(old_capacity != capacity) {
            move_to_new();
        }
    }

    void move_to_new()
    {
        T *new_data = new T[capacity];
        for(size_t i = 0; i < size; ++i) { // slow for bytes, add a trait later to use memcpy for integral types
            new_data[i] = data[i];
        }
        delete[] data;
        data = new_data;
    }
};
