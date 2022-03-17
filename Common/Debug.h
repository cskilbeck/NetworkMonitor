//////////////////////////////////////////////////////////////////////

#pragma once

namespace
{
    template <typename T> struct _fmt
    {
        static constexpr T const *empty_string = null;
        static size_t length(T const *p) = delete;
        static size_t count(T const *p, va_list v) = delete;
        static size_t print(T *const b, size_t const c, size_t const m, T const *const f, va_list a) = delete;
        static bool compare(T const *a, T const *b) = delete;
        static bool compare_nocase(T const *a, T const *b) = delete;
        static bool compare_len(T const *a, T const *b, size_t len) = delete;
        static T const *find(T const *n, T const *h) = delete;
        static T const *find_chr(T const *n, T h) = delete;
        static T *alloc(size_t n) = delete;
    };

    template <> struct _fmt<char>
    {
        static constexpr char const *empty_string = "";

        static size_t length(char const *p)
        {
            return strlen(p);
        }
        static size_t count(char const *p, va_list v)
        {
            return _vscprintf(p, v);
        }
        static size_t print(char *const b, size_t const c, size_t const m, char const *const f, va_list a)
        {
            return _vsnprintf_s(b, c, m, f, a);
        }
        static bool compare(char const *a, char const *b)
        {
            return strcmp(a, b);
        }
        static bool compare_nocase(char const *a, char const *b)
        {
            return _stricmp(a, b);
        }
        static bool compare_len(char const *a, char const *b, size_t len)
        {
            return strncmp(a, b, len);
        }
        static char const *find(char const *n, char const *h)
        {
            return strstr(n, h);
        }
        static char const *find_chr(char const *n, char h)
        {
            return strchr(n, h);
        }
        static char *alloc(size_t n)
        {
            return (char *)malloc(n + 1);
        }
        static bool is_whitespace(char c)
        {
            return strchr(" \t\r\n", c) != null;
        }
    };

    template <> struct _fmt<wchar>
    {
        static constexpr wchar const *empty_string = L"";

        static size_t length(wchar const *p)
        {
            return wcslen(p);
        }
        static size_t count(wchar const *p, va_list v)
        {
            return _vscwprintf(p, v);
        }
        static size_t print(wchar *const b, size_t const c, size_t const m, wchar const *const f, va_list a)
        {
            return _vsnwprintf_s(b, c, m, f, a);
        }
        static bool compare(wchar const *a, wchar const *b)
        {
            return wcscmp(a, b);
        }
        static bool compare_nocase(wchar const *a, wchar const *b)
        {
            return _wcsicmp(a, b);
        }
        static bool compare_len(wchar const *a, wchar const *b, size_t len)
        {
            return wcsncmp(a, b, len);
        }
        static wchar const *find(wchar const *n, wchar const *h)
        {
            return wcsstr(n, h);
        }
        static wchar const *find_chr(wchar const *n, wchar h)
        {
            return wcschr(n, h);
        }
        static wchar *alloc(size_t n)
        {
            return (wchar *)malloc((n + 1) * sizeof(wchar));
        }
    };
} // namespace

//////////////////////////////////////////////////////////////////////

template <typename T> class str_base
{
    using F = _fmt<T>;

public:
    str_base()
        : buf(null)
    {
    }

    str_base(size_t s)
        : buf(null)
    {
        if(s != 0) {
            buf = F::alloc(s);
        }
    }

    str_base(T const *p)
        : buf(null)
    {
        assign(p);
    }

    str_base(str_base &&other)
        : buf(other.buf)
    {
        other.buf = null;
    }

    str_base &operator=(str_base &&other)
    {
        if(this != &other) {
            if(buf != null) {
                free(buf);
            }
            buf = other.buf;
            other.buf = null;
        }
        return *this;
    }

    bool empty() const
    {
        return buf == null || buf[0] == 0;
    }

    T *buffer() const
    {
        return buf;
    }

    // naughty but nice
    operator T const *() const
    {
        return empty() ? F::empty_string : buf;
    }

    // need this if passing into a variadic function and it's wide, which is a damn shame...
    T const *c_str() const
    {
        return empty() ? F::empty_string : buf;
    }

    void clear()
    {
        if(buf != null) {
            free(buf);
            buf = null;
        }
    }

    ~str_base()
    {
        clear();
    }

    // capacity is size in bytes of the buffer
    size_t capacity() const
    {
        return buf == null ? 0 : _msize(buf);
    }

    // size is number of chars (uses strlen)
    size_t size() const
    {
        if(buf == null) {
            return 0;
        }
        return F::length(buf);
    }

    str_base substr(uintptr_t offset, uintptr_t length) const
    {
        size_t l = size();
        if(offset >= l) {
            return str_base();
        }
        if(offset + length >= l) {
            length = l - offset;
        }
        str_base n(length);
        copy(n.buffer(), buffer() + offset, length);
        n.buffer()[length] = 0;
        return n;
    }

    static str_base format(T const *fmt, va_list v)
    {
        str_base n;
        size_t l = F::count(fmt, v);
        if(l > 0) {
            n.resize(l);
            F::print((T *)n.buffer(), l + 1, l, fmt, v);
        }
        return n;
    }

    static str_base format(T const *fmt, ...)
    {
        va_list v;
        va_start(v, fmt);
        return format(fmt, v);
    }

    void resize(size_t s, bool keep = false)
    {
        size_t old_size = size();
        T *new_buf = null;
        if(s != 0) {
            new_buf = F::alloc(s);
        }
        if(keep && new_buf != null && buf != null) {
            if(old_size < s) {
                old_size = s;
            }
            copy(new_buf, buf, old_size + 1);
        }
        if(buf != null) {
            free(buf);
            buf = null;
        }
        buf = new_buf;
    }

    static void copy(T *dst, T const *src, size_t count)
    {
        memcpy(dst, src, count * sizeof(T));
    }

    str_base duplicate() const
    {
        size_t s = size();
        str_base t(s);
        if(s != 0) {
            copy(t.buffer(), buf, s + 1);
        }
        return t;
    }

    void assign(str_base const &other)
    {
        size_t l = other.size();
        resize(l);
        copy(buf, other, l + 1);
    }

    str_base append(str_base const &other)
    {
        size_t l1 = size();
        size_t l2 = other.size();
        str_base n(l1 + l2 + 1);
        copy(n.buffer(), c_str(), l1);
        copy(n.buffer() + l1, other.c_str(), l2);
        n.buffer()[l1 + l2] = 0;
        return n;
    }

    bool compare(str_base const &other)
    {
        return F::compare(*this, other);
    }

    bool compare_i(str_base const &other)
    {
        return F::compare_nocase(*this, other);
    }

    T back() const
    {
        return empty() ? 0 : buf[size() - 1];
    }

    str_base &assign(T const *s)
    {
        size_t l = F::length(s);
        resize(l);
        if(l != 0) {
            copy(buf, s, l + 1);
        }
        return *this;
    }

    // no lvalue copying! use duplicate!
    str_base &operator=(str_base &other) = delete;

    // not for general use!
    void set_buffer(T *buffer)
    {
        buf = buffer;
    }

private:
    T *buf;
};

//////////////////////////////////////////////////////////////////////

using str = str_base<char>;
using wstr = str_base<wchar_t>;
using tstr = str_base<tchar>;

//////////////////////////////////////////////////////////////////////

template <typename T> inline str_base<T> operator+(str_base<T> const &a, str_base<T> const &b)
{
    size_t len = a.size() + b.size();
    str_base<T> n(len);
    str_base<T>::copy(n.buffer(), a, a.size());
    str_base<T>::copy(n.buffer() + a.size(), b, b.size());
    n.buffer()[len] = 0;
    return n;
}

//////////////////////////////////////////////////////////////////////

template <typename T> inline str_base<T> $(T const *fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    return str_base<T>::format(fmt, v);
}

//////////////////////////////////////////////////////////////////////

template <typename T> inline str_base<T> TrimStr(str_base<T> const &s)
{
    int l = s.size();
    if(l == 0) {
        return s.duplicate();
    }
    int start = 0;
    int end = l - 1;
    T const *p = s.buffer();
    while(_fmt<T>::is_whitespace(p[start]) && start < l) {
        start += 1;
    }
    while(end > start && _fmt<T>::is_whitespace(p[end])) {
        end -= 1;
    }
    return s.substr(start, end - start + 1);
}

//////////////////////////////////////////////////////////////////////

inline wstr GetWindowsError(DWORD err, wchar const *message, va_list v)
{
    wstr msg = wstr::format(message, v); // eat the format specifiers
    msg = $(L"%s: ", msg.c_str());       // add <colon><space>
    LPTSTR lpMsgBuf;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessage(flags, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    wstr s =
        $(L"%s%08x%s", msg.c_str(), err, (lpMsgBuf != null && lpMsgBuf[0] != 0) ? $(L" - %s", lpMsgBuf).c_str() : L"");
    LocalFree(lpMsgBuf);
    if(s.back() == '\n') {
    }
    return s;
}

//////////////////////////////////////////////////////////////////////

inline wstr WindowsError(DWORD err, wchar const *message, ...)
{
    va_list v;
    va_start(v, message);
    return GetWindowsError(err, message, v);
}

//////////////////////////////////////////////////////////////////////

inline wstr WindowsError(wchar const *message, ...)
{
    va_list v;
    va_start(v, message);
    return GetWindowsError(GetLastError(), message, v);
}

//////////////////////////////////////////////////////////////////////

inline wstr StrToWide(str const &s)
{
    wstr temp;
    size_t bufSize = MultiByteToWideChar(CP_UTF8, 0, s, (int)s.size(), null, 0);
    if(bufSize > 0) {
        temp.resize(bufSize);
        int got = MultiByteToWideChar(CP_UTF8, 0, s, (int)s.size(), temp.buffer(), (int)bufSize);
        temp.buffer()[got] = 0;
    }
    return temp;
}

//////////////////////////////////////////////////////////////////////

inline str WStrToNarrow(wstr const &s)
{
    str temp;
    int bufSize = WideCharToMultiByte(CP_UTF8, 0, s, (int)s.size(), NULL, 0, NULL, FALSE);
    if(bufSize > 0) {
        temp.resize(bufSize);
        int got = WideCharToMultiByte(CP_UTF8, 0, s, (int)s.size(), temp.buffer(), bufSize, NULL, FALSE);
        temp.buffer()[got] = 0;
    }
    return temp;
}

//////////////////////////////////////////////////////////////////////

inline wstr GetCurrentExeW()
{
    wchar *name;
    _get_wpgmptr(&name);
    return name;
}

inline str GetCurrentExeA()
{
    char *name;
    _get_pgmptr(&name);
    return name;
}

#if defined(UNICODE)
#define GetCurrentExe GetCurrentExeW
#else
#define GetCurrentExe GetCurrentExeA
#endif

//////////////////////////////////////////////////////////////////////

inline wstr DoubleSlash(wstr const &src)
{
    size_t l = src.size();
    size_t len = l + 1;
    for(size_t i = 0; i < l; ++i) {
        if(src[i] == '\\') {
            len += 1;
        }
    }
    wstr temp(len);
    wchar *p = temp.buffer();
    for(size_t i = 0; i < l; ++i) {
        if(src[i] == '\\') {
            *p++ = '\\';
        }
        *p++ = src[i];
    }
    *p++ = 0;
    return temp;
}

//////////////////////////////////////////////////////////////////////

template <typename T> inline str_base<T> RemoveChr(str_base<T> const &s, T what)
{
    size_t len = s.size();
    str_base<T> n(len);
    T *p = n.buffer();
    for(size_t i = 0; i < len; ++i) {
        T got = s[i];
        if(got != what) {
            *p++ = got;
        }
    }
    *p++ = 0;
    return n;
}

//////////////////////////////////////////////////////////////////////

template <typename T> inline str_base<T> RemoveChrs(str_base<T> const &s, T const *what)
{
    using F = _fmt<T>;
    size_t len = s.size();
    str_base<T> n(len);
    T *p = n.buffer();
    for(size_t i = 0; i < len; ++i) {
        T got = s[i];
        if(F::find_chr(what, got) == nullptr) {
            *p++ = got;
        }
    }
    *p++ = 0;
    return n;
}

//////////////////////////////////////////////////////////////////////

template <typename T> inline bool StrEqual(str_base<T> const &a, str_base<T> const &b)
{
    return _fmt<T>::compare(a, b) == 0;
}

//////////////////////////////////////////////////////////////////////

template <typename T> inline str_base<T> ReplaceStr(str_base<T> const &s, T what, T with)
{
    size_t len = s.size();
    str_base<T> n(len);
    T *p = n.buffer();
    for(size_t i = 0; i < len; ++i) {
        T got = s[i];
        if(got == what) {
            got = with;
        }
        *p++ = got;
    }
    *p++ = 0;
    return n;
}

//////////////////////////////////////////////////////////////////////

template <typename T>
inline str_base<T> ReplaceStr(str_base<T> const &s, str_base<T> const &what, str_base<T> const &with)
{
    using F = _fmt<T>;

    if(what.empty()) {
        str_base<T> n(s.c_str());
        return n;
    }
    size_t pos = 0;
    size_t what_len = what.size();
    size_t with_len = with.size();
    size_t s_len = s.size();

    size_t total = s_len;                // length of new string
    intptr_t diff = with_len - what_len; // delta size with each replacement

    // count occurrences
    for(size_t pos = 0; pos < total;) {
        T const *p = s.c_str() + pos;
        T const *offset = F::find(p, what);
        if(offset == null) {
            break;
        }
        total += diff;
        pos += (p - offset) + what_len;
    }

    if(total == 0) {
        return str_base<T>();
    }

    // build new one
    str_base<T> new_str(total + 1);
    T *new_p = new_str.buffer();
    T *end_p = new_p + s_len;
    for(size_t pos = 0; pos < total;) {
        T const *p = s.c_str() + pos;
        T const *offset =
            F::find(p, what); // find again, don't want to store a list of offsets, where would it be kept?
        intptr_t diff = offset - p;
        if(offset != null) {
            // found one, copy in previous bit and replacement
            str_base<T>::copy(new_p, p, diff);
            new_p += diff;
            str_base<T>::copy(new_p, with, with_len);
            new_p += with_len;
            pos += diff + what_len;
        }
        else {
            // no more, copy last bit
            size_t extra = s_len - pos;
            str_base<T>::copy(new_p, p, extra);
            new_p += extra;
            break;
        }
    }
    *new_p++ = 0;
    return new_str;
}

//////////////////////////////////////////////////////////////////////

inline wstr ForwardSlashes(wstr const &s)
{
    return ReplaceStr(s, L'\\', L'/');
}

//////////////////////////////////////////////////////////////////////

inline wstr SingleSlashes(wstr const &s)
{
    int extra = 0;
    size_t len = s.size();
    for(size_t i = 0; i < len; ++i) {
        if(s[i] == '\\' && s[i + 1] == '\\') {
            extra += 1;
        }
    }
    wstr new_str(s.size() - extra);
    wchar prev = 0;
    wchar *p = new_str.buffer();
    for(size_t i = 0; i < s.size(); ++i) {
        if(s[i] != '\\' || prev != '\\') {
            *p++ = s[i];
        }
        prev = s[i];
    }
    *p++ = 0;
    return new_str;
}

//////////////////////////////////////////////////////////////////////

inline wstr ExpandEnvStrings(wstr const &src)
{
    DWORD size = ExpandEnvironmentStringsW(src, null, 0);
    wstr result(size + 2);
    ExpandEnvironmentStringsW(src, result.buffer(), size);
    return result;
}

//////////////////////////////////////////////////////////////////////

inline void ReportWindowsError(wchar const *message, ...)
{
    va_list v;
    va_start(v, message);
    fwprintf(stderr, L"Error:%s\n", GetWindowsError(GetLastError(), message, v).c_str());
}

//////////////////////////////////////////////////////////////////////

int GetCurrentWindowsVersion(bool &server);
int ConvertStringToWindowsVersion(wstr const &version);
wstr ConvertWindowsVersionToString(int version, bool server);
bool HexStringToBytes(char const *hex, int len, byte *output);
wstr BytesToHexString(byte const *bytes, int len);
