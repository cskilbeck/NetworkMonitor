//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

/*

Version

|Version |Client         |Server
|--------|-----------------------------------------|
|10.0    |Windows 10     |Windows Server 2016      |
|6.3     |Windows 8.1    |Windows Server 2012 R2   |
|6.2     |Windows 8      |Windows Server 2012      |
|6.1     |Windows 7      |Windows Server 2008 R2   |
|6.0     |Windows Vista  |Windows Server 2008      |
|5.2     |               |Windows Server 2003 [R2] |
|5.1     |Windows XP     |                         |
|5.0     |Windows 2000   |                         |

Service pack major = 1 for SP1, 2 for SP2 etc
Service pack minor seems to be 0

Server
true   Windows Server
true   Domain controller
false  Windows Client

*/

struct WindowsVersion
{
    int major;
    int minor;
    wchar const *client_name;
    wchar const *server_name;
};

// clang-format off
static WindowsVersion windows_versions[] = {
    { 5, 0, L"2000",  L"?" },
    { 5, 1, L"XP",    L"?" },
    { 5, 2, L"?",     L"Server 2003" },
    { 6, 0, L"Vista", L"Server 2008" },
    { 6, 1, L"7",     L"Server 2008 R2" },
    { 6, 2, L"8",     L"Server 2012" },
    { 6, 3, L"8.1",   L"Server 2012 R2" },
    { 10, 0, L"10",   L"Server 2016" }
};
// clang-format on

//////////////////////////////////////////////////////////////////////

static wchar const *FindWindowsVersion(int version, bool server)
{
    int major = version / 100;
    int minor = (version / 10) % 10;
    for(auto const &v : windows_versions) {
        if(v.major == major && v.minor == minor) {
            return server ? v.server_name : v.client_name;
        }
    }
    return L"??";
}

//////////////////////////////////////////////////////////////////////

int ConvertStringToWindowsVersion(wstr const &version)
{
    int major = 0;
    int minor = 0;
    int sp_major = 0;
    wstr v = ReplaceStr(version, (wchar)'.', (wchar)';');
    wstr r(L"Unknown");
    wchar *ender;
    major = wcstoul(v, &ender, 10);
    if(major != 0) {
        if(*ender != 0) {
            ++ender;
            wchar *ender2;
            minor = wcstoul(ender, &ender2, 10);
            if(minor != 0 && *ender2 == ';') {
                ++ender2;
                wchar *ender3;
                sp_major = wcstoul(ender2, &ender3, 10);
                return major * 100 + minor * 10 + sp_major;
            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////

wstr ConvertWindowsVersionToString(int version, bool server)
{
    wchar const *name = FindWindowsVersion(version, server);
    wstr sp(L"");
    if((version % 10) != 0) {
        sp = $(L" Service Pack %d", version % 10);
    }
    return $(L"Windows %s%s", name, sp.c_str());
}

//////////////////////////////////////////////////////////////////////

bool GetCurrentWindowsVersionName(wstr &s)
{
    bool is_server;
    int current_version = GetCurrentWindowsVersion(is_server);
    if(current_version == 0) {
        return wstr();
    }
    if(current_version >= 1000) {
        return L"Windows 10";
    }
    s = ConvertWindowsVersionToString(current_version, is_server);
    return true;
}

//////////////////////////////////////////////////////////////////////

int GetCurrentWindowsVersion(bool &server)
{
    int major = 0;
    int minor = 0;
    int service_pack_major = 0;
    typedef LONG(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if(hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if(fxPtr != nullptr) {
            RTL_OSVERSIONINFOEXW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if(fxPtr(&rovi) == 0) {
                major = rovi.dwMajorVersion;
                minor = rovi.dwMinorVersion;
                service_pack_major = rovi.wServicePackMajor;
                server = rovi.wProductType != VER_NT_WORKSTATION;
                return major * 100 + minor * 10 + service_pack_major;
            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////
// len is # of hex digits in the string (so, double the # of bytes)

bool HexStringToBytes(char const *hex, int len, byte *output)
{
    byte value = 0;
    for(int i = 0; i < len; ++i) {
        char h = hex[i];
        if(h == 0) {
            return false;
        }
        if(h >= 'A' && h <= 'F') {
            h -= 'A' - 10;
        }
        else if(h >= 'a' && h <= 'f') {
            h -= 'a' - 10;
        }
        else if(h >= '0' && h <= '9') {
            h -= '0';
        }
        else {
            return false;
        }
        value <<= 4;
        value |= h;
        if((i & 1) != 0) {
            *output++ = value;
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////
// len is # of hex digits in the string (so, double the # of bytes)

wstr BytesToHexString(byte const *bytes, int len)
{
    wstr r(len * 2 + 1);
    for(int i = 0; i < len; ++i) {
        wsprintf(r.buffer() + i * 2, L"%02x", bytes[i]);
    }
    r.buffer()[len * 2] = 0;
    return r;
}
