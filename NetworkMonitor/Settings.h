//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

struct settings_struct
{
    uint32 bar_width = 5;
    uint32 bar_gap = 1;

    double bars_per_second = 10; // scroll speed

    double min_scale_mbps = 1;
    double max_scale_mbps = 1000;

    double scale_up_rate = 0.1;
    double scale_down_rate = 0.01;

    uint32 window_borderless = 0;
    uint32 window_topmost = 0;
    uint32 window_transparency = 255;
    uint32 hide_taskbar_icon = 0;

    // client rect in screen coords
    uint32 window_x = 300;
    uint32 window_y = 300;
    uint32 window_w = 400;
    uint32 window_h = 400;

    D2D1_COLOR_F in_bar_color{ 1, 0, 0, 0.5f };
    D2D1_COLOR_F out_bar_color{ 0, 0.2f, 1, 0.5f };
    D2D1_COLOR_F text_color{ 1.0f, 1.0f, 1.0f, 0.6f };
    D2D1_COLOR_F line_color{ 1.0f, 1.0f, 1.0f, 0.4f };
    D2D1_COLOR_F bg_color{ 0.0f, 0.0f, 0.0f, 1.0f };

    GUID network_adapter_guid{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    HRESULT save() const;
    HRESULT load();
};
