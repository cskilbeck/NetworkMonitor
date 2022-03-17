#pragma once

struct Timer
{
    uint64 t;

    Timer() = default;

    void reset()
    {
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&t));
    }

    double elapsed()
    {
        uint64 n;
        uint64 f;
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&n));
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&f));
        return static_cast<double>(n - t) / f;
    }
};
