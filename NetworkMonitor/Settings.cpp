//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

namespace
{
    HRESULT save_string(HKEY key, wchar const *name, std::wstring const &str)
    {
        HR(RegSetValueExW(
            key, name, 0, REG_SZ, reinterpret_cast<byte const *>(str.data()), static_cast<DWORD>(str.size())));
        return S_OK;
    }

    HRESULT save_color(HKEY key, wchar const *name, D2D1_COLOR_F const &color)
    {
        HR(RegSetValueExW(key, name, 0, REG_BINARY, reinterpret_cast<byte const *>(&color), sizeof(D2D1_COLOR_F)));
        return S_OK;
    }

    HRESULT save_uint32(HKEY key, wchar const *name, uint32 value)
    {
        HR(RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<byte const *>(&value), sizeof(uint32)));
        return S_OK;
    }

    HRESULT save_double(HKEY key, wchar const *name, double value)
    {
        HR(RegSetValueExW(key, name, 0, REG_BINARY, reinterpret_cast<byte const *>(&value), sizeof(double)));
        return S_OK;
    }

    HRESULT save_guid(HKEY key, wchar const *name, GUID const &value)
    {
        HR(RegSetValueExW(key, name, 0, REG_BINARY, reinterpret_cast<byte const *>(&value), sizeof(GUID)));
        return S_OK;
    }

    HRESULT load_string(HKEY key, wchar const *name, std::wstring &str, std::wstring const &default_value)
    {
        DWORD size;
        HR(RegQueryValueExW(key, name, null, null, null, &size));
        std::wstring result(size, 0);
        if(FAILED(RegQueryValueExW(key, name, null, null, reinterpret_cast<byte *>(&result[0]), &size))) {
            str = default_value;
            return ERROR_NOT_FOUND;
        }
        return S_OK;
    }

    HRESULT load_color(HKEY key, wchar const *name, D2D1_COLOR_F &color, D2D1_COLOR_F const &default_value)
    {
        DWORD size{ sizeof(D2D1_COLOR_F) };
        if(FAILED(RegQueryValueExW(key, name, null, null, reinterpret_cast<byte *>(&color), &size))) {
            color = default_value;
            return ERROR_NOT_FOUND;
        }
        return S_OK;
    }

    HRESULT load_uint32(HKEY key, wchar const *name, uint32 &value, uint32 default_value)
    {
        DWORD size{ sizeof(uint32) };
        if(FAILED(RegQueryValueExW(key, name, null, null, reinterpret_cast<byte *>(&value), &size))) {
            value = default_value;
            return ERROR_NOT_FOUND;
        }
        return S_OK;
    }

    HRESULT load_double(HKEY key, wchar const *name, double &value, double default_value)
    {
        DWORD size{ sizeof(double) };
        if(FAILED(RegQueryValueExW(key, name, null, null, reinterpret_cast<byte *>(&value), &size))) {
            value = default_value;
            return ERROR_NOT_FOUND;
        }
        return S_OK;
    }

    HRESULT load_guid(HKEY key, wchar const *name, GUID &value, GUID const &default_value)
    {
        DWORD size{ sizeof(GUID) };
        if(FAILED(RegQueryValueExW(key, name, null, null, reinterpret_cast<byte *>(&value), &size))) {
            value = default_value;
            return ERROR_NOT_FOUND;
        }
        return S_OK;
    }
}

//////////////////////////////////////////////////////////////////////

// HKCU\\Software\\NetworkMonitor

HRESULT settings_struct::save() const
{
    HKEY key;
    HR(RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\NetworkMonitor", &key));
    defer(RegCloseKey(key));

    save_color(key, L"in_bar_color", in_bar_color);
    save_color(key, L"out_bar_color", out_bar_color);
    save_color(key, L"text_color", text_color);
    save_color(key, L"line_color", line_color);
    save_color(key, L"bg_color", bg_color);
    save_uint32(key, L"bar_width", bar_width);
    save_uint32(key, L"bar_gap", bar_gap);
    save_double(key, L"bars_per_second", bars_per_second);
    save_uint32(key, L"window_borderless", window_borderless);
    save_uint32(key, L"window_topmost", window_topmost);
    save_uint32(key, L"window_transparency", window_transparency);
    save_uint32(key, L"window_x", window_x);
    save_uint32(key, L"window_y", window_y);
    save_uint32(key, L"window_w", window_w);
    save_uint32(key, L"window_h", window_h);
    save_double(key, L"min_scale_mbps", min_scale_mbps);
    save_double(key, L"max_scale_mbps", max_scale_mbps);
    save_double(key, L"scale_up_rate", scale_up_rate);
    save_double(key, L"scale_down_rate", scale_down_rate);
    save_guid(key, L"adapter", network_adapter_guid);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT settings_struct::load()
{
    HKEY key;
    HR(RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\NetworkMonitor", &key));
    defer(RegCloseKey(key));

    load_color(key, L"in_bar_color", in_bar_color, in_bar_color);
    load_color(key, L"out_bar_color", out_bar_color, out_bar_color);
    load_color(key, L"text_color", text_color, text_color);
    load_color(key, L"line_color", line_color, line_color);
    load_color(key, L"bg_color", bg_color, bg_color);
    load_uint32(key, L"bar_width", bar_width, bar_width);
    load_uint32(key, L"bar_gap", bar_gap, bar_gap);
    load_double(key, L"bars_per_second", bars_per_second, bars_per_second);
    load_uint32(key, L"window_borderless", window_borderless, window_borderless);
    load_uint32(key, L"window_topmost", window_topmost, window_topmost);
    load_uint32(key, L"window_transparency", window_transparency, window_transparency);
    load_uint32(key, L"window_x", window_x, window_x);
    load_uint32(key, L"window_y", window_y, window_y);
    load_uint32(key, L"window_w", window_w, window_w);
    load_uint32(key, L"window_h", window_h, window_h);
    load_double(key, L"min_scale_mbps", min_scale_mbps, min_scale_mbps);
    load_double(key, L"max_scale_mbps", max_scale_mbps, max_scale_mbps);
    load_double(key, L"scale_up_rate", scale_up_rate, scale_up_rate);
    load_double(key, L"scale_down_rate", scale_down_rate, scale_down_rate);
    load_guid(key, L"adapter", network_adapter_guid, { 0 });
    return S_OK;
}
