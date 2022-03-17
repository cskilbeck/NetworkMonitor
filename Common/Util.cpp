//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

std::wstring get_error_message(DWORD err)
{
    if(err == 0) {
        err = GetLastError();
    }
    wchar *lpMsgBuf;
    auto options = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    auto language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    FormatMessageW(options, NULL, err, language, reinterpret_cast<wchar *>(&lpMsgBuf), 0, NULL);
    std::wstring s(lpMsgBuf);
    LocalFree(lpMsgBuf);
    return s;
}

//////////////////////////////////////////////////////////////////////

std::string format_valist(char const *fmt, va_list v)
{
    using std::string;
    static char buffer[512];
    int l = _vsnprintf_s(buffer, _countof(buffer), _TRUNCATE, fmt, v);
    if(l != -1) {
        return string(buffer);
    }
    l = _vscprintf(fmt, v) + 1;
    string s(l, 0);
    _vsnprintf_s(&s[0], l, _TRUNCATE, fmt, v);
    return s;
}

//////////////////////////////////////////////////////////////////////

std::wstring format_valist_wide(wchar_t const *fmt, va_list v)
{
    using std::wstring;
    static wchar_t buffer[512];
    int l = _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, fmt, v);
    if(l != -1) {
        return wstring(buffer);
    }
    l = _vscwprintf(fmt, v) + 1;
    wstring s(l, 0);
    _vsnwprintf_s(&s[0], l, _TRUNCATE, fmt, v);
    return s;
}

//////////////////////////////////////////////////////////////////////

std::string format(char const *fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    return format_valist(fmt, v);
}

//////////////////////////////////////////////////////////////////////

std::wstring format(wchar_t const *fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    return format_valist_wide(fmt, v);
}

//////////////////////////////////////////////////////////////////////

void trace(wchar const *fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    std::wstring s = format_valist_wide(fmt, v);
    OutputDebugStringW(s.c_str());
}

//////////////////////////////////////////////////////////////////////

void trace(char const *fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    std::string s = format_valist(fmt, v);
    OutputDebugStringA(s.c_str());
}

//////////////////////////////////////////////////////////////////////

std::wstring load_resource_string(UINT id, wchar const *default_value)
{
    wchar *resource;
    auto len = LoadStringW(GetModuleHandle(NULL), id, reinterpret_cast<wchar *>(&resource), 0);
    if(len == 0) {
        trace(L"Can't load string resource %d: %s\n", id, get_error_message().c_str());
        return default_value;
    }
    return std::wstring(resource, len);
}

//////////////////////////////////////////////////////////////////////

std::wstring get_build_version()
{
    char buffer[512];
    int const buffer_size = _countof(buffer);
    if(GetModuleFileNameA(NULL, buffer, buffer_size) >= buffer_size) {
        trace(L"GetModuleFileName too long\n");
        return L"?.?";
    }
    DWORD verHandle = 0;
    LPBYTE lpBuffer = NULL;
    DWORD verSize = GetFileVersionInfoSizeA(buffer, &verHandle);
    if(verSize == NULL) {
        trace(L"GetFileVersionInfoSize failed\n");
        return L"?.?";
    }
    char *verData = new char[verSize];
    defer(delete[] verData);
    if(!GetFileVersionInfoA(buffer, verHandle, verSize, verData)) {
        trace(L"GetFileVersionInfoSize failed\n");
        return L"?.?";
    }
    UINT size = 0;
    if(!VerQueryValueA(verData, "\\", (VOID FAR * FAR *)&lpBuffer, &size)) {
        trace(L"VerQueryValue failed\n");
        return L"?.?";
    }
    if(!size) {
        trace(L"Bad version size!?\n");
        return L"?.?";
    }
    VS_FIXEDFILEINFO &v = *(VS_FIXEDFILEINFO *)lpBuffer;
    if(v.dwSignature != 0xfeef04bd) {
        trace(L"Bad version ID!?\n");
        return L"?.?";
    }
    return format(L"%d.%d", (v.dwProductVersionMS >> 16) & 0xffff, v.dwProductVersionMS & 0xffff);
}
