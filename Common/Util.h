#pragma once

//////////////////////////////////////////////////////////////////////

std::wstring get_error_message(DWORD err = 0);

std::string format_valist(char const *fmt, va_list v);
std::wstring format_valist_wide(wchar_t const *fmt, va_list v);

std::string format(char const *fmt, ...);
std::wstring format(wchar_t const *fmt, ...);

void trace(wchar const *fmt, ...);
void trace(char const *fmt, ...);

std::wstring load_resource_string(UINT id, wchar const *default_value = L"??");
std::wstring get_build_version();

//////////////////////////////////////////////////////////////////////

template <typename T> void com_release(T *&ptr)
{
    if(ptr != null) {
        ptr->Release();
        ptr = null;
    }
}

//////////////////////////////////////////////////////////////////////

inline void replace_str(std::wstring &str, std::wstring const &find, std::wstring const &replace)
{
    std::string::size_type n = 0;
    while((n = str.find(find, n)) != std::string::npos) {
        str.replace(n, find.size(), replace);
        n += replace.size();
    }
}

//////////////////////////////////////////////////////////////////////

inline std::wstring wide_string_from_string(std::string const &str)
{
    int n = MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.size()), null, 0);
    if(n > 0) {
        std::wstring w(n, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.size()), &w[0], n);
        return w;
    }
    return L"?";
}

//////////////////////////////////////////////////////////////////////

struct rect : RECT
{
    rect() = default;

    explicit rect(int x, int y, int width, int height)
    {
        left = x;
        right = x + width;
        top = y;
        bottom = y + height;
    }

    int width() const
    {
        return right - left;
    }

    int height() const
    {
        return bottom - top;
    }

    void clamp_min_size(int w, int h)
    {
        if(width() < w) {
            right = left + w;
        }
        if(height() < h) {
            bottom = top + h;
        }
    }
};

//////////////////////////////////////////////////////////////////////

#define HR(x)               \
    {                       \
        HRESULT __hr = (x); \
        if(FAILED(__hr)) {  \
            return __hr;    \
        }                   \
    }

//////////////////////////////////////////////////////////////////////

inline double calc_bandwidth(uint64 total_bytes, double timespan)
{
    if(timespan == 0) {
        return 0;
    }
    return total_bytes / timespan / 125000; // 125000 = bytes per mbps (1 million / 8)
}

//////////////////////////////////////////////////////////////////////

inline double time_now()
{
    uint64 ticks;
    uint64 frequency;
    QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
    QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
    return static_cast<double>(ticks) / frequency;
}

//////////////////////////////////////////////////////////////////////

struct timer
{
    double _start;

    timer()
    {
        _start = time_now();
    }

    double elapsed()
    {
        return time_now() - _start;
    }
};
