//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

// sysmenu disappears when borderless toggled
// borderless toggle changes window size on some DPI displays
// window topmost flag not working sometimes

//////////////////////////////////////////////////////////////////////

LOG_Context("WinMain");

// global settings
settings_struct settings;

namespace
{
    using std::wstring;

    // win32 admin
    HINSTANCE instance_handle;
    wstring window_title;
    wstring window_class_name;
    HMENU window_menu;
    HMENU popup_menu;
    HWND main_hwnd;

    // window style admin
    uint32 border_window_style = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME;
    uint32 borderless_window_style = WS_POPUP | WS_BORDER;

    // settings
    settings_struct old_settings;

    // network graph stuff
    network_stats net_stats(1.0 / settings.bars_per_second, 2000);
    graph_renderer renderer(net_stats);
    MIB_IF_TABLE2 *all_adapters = null;
    MIB_IF_ROW2 current_adapter = { 0 };
    bool adapter_valid = false;
    bool adapter_changed = false;

    // notify icon stuff
    UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;

    // notify icon guid tracks exe path, fails if it changes (add more if building 32 & 64 bit)
#if defined(_DEBUG)
    class __declspec(uuid("624E4C39-F777-491E-94D2-C972685B496E")) NotifyIconGUID;
#else
    class __declspec(uuid("C72F0FD8-4DEE-4282-B76E-79EB694D8CF6")) NotifyIconGUID;
#endif

    // bogus hit test stuff for borderless window
    uint32 hits[3][3] = {

        { HTTOPLEFT, HTTOP, HTTOPRIGHT },
        { HTLEFT, HTCAPTION, HTRIGHT },
        { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT }
    };

    //////////////////////////////////////////////////////////////////////

    uint32 nc_hit_test(HWND hwnd, uint message, WPARAM wparam, LPARAM lparam)
    {
        if(!settings.window_borderless) {
            return DefWindowProc(hwnd, message, wparam, lparam);
        }

        int drag_border = 12;

        rect wr;
        GetClientRect(hwnd, &wr);
        wr.right -= drag_border;
        wr.bottom -= drag_border;

        POINT p{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
        ScreenToClient(hwnd, &p);

        int horiz = 1;
        int vert = 1;

        if(p.x < drag_border) {
            horiz = 0;
        }
        if(p.x > wr.right) {
            horiz = 2;
        }
        if(p.y < drag_border) {
            vert = 0;
        }
        if(p.y > wr.bottom) {
            vert = 2;
        }
        return hits[vert][horiz];
    }

    //////////////////////////////////////////////////////////////////////

    void setup_trackbar(HWND hdlg, int bar_id, int min_value, int max_value, int cur_value, int static_id)
    {
        HWND tb = GetDlgItem(hdlg, bar_id);
        SendMessage(tb, TBM_SETRANGE, 0, min_value | (max_value << 16));
        Static_SetText(GetDlgItem(hdlg, static_id), format(L"%d", cur_value).c_str());
        SendMessage(tb, TBM_SETPOS, true, cur_value);
    }

    //////////////////////////////////////////////////////////////////////

    void report_trackbar(HWND hdlg, int bar_id, int &value, int static_id)
    {
        value = SendMessage(GetDlgItem(hdlg, bar_id), TBM_GETPOS, 0, 0);
        Static_SetText(GetDlgItem(hdlg, static_id), format(L"%d", value).c_str());
    }

    //////////////////////////////////////////////////////////////////////

    void insert_listview_column(HWND lv, wchar const *title, int index, int width)
    {
        LVCOLUMN column;
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = width;
        column.pszText = const_cast<wchar *>(title);
        ListView_InsertColumn(lv, index, &column);
    }

    //////////////////////////////////////////////////////////////////////

    void add_notification_icon(HWND hwnd)
    {
        NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATAA_V2_SIZE) };
        nid.hWnd = hwnd;
        nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_GUID;
        LoadIconMetric(instance_handle, MAKEINTRESOURCE(IDI_MAINICON), LIM_SMALL, &nid.hIcon);
        LoadString(instance_handle, IDS_TOOLTIP, nid.szTip, _countof(nid.szTip));
        nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
        nid.guidItem = __uuidof(NotifyIconGUID);
        if(!Shell_NotifyIconW(NIM_ADD, &nid)) {
            trace(L"Shell_NotifyIcon failed: %s\n", get_error_message().c_str());
        }
    }

    //////////////////////////////////////////////////////////////////////

    void delete_notification_icon(HWND hwnd)
    {
        NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATAA_V2_SIZE) };
        nid.uFlags = NIF_GUID;
        nid.guidItem = __uuidof(NotifyIconGUID);
        nid.hWnd = hwnd;
        if(!Shell_NotifyIcon(NIM_DELETE, &nid)) {
            trace(L"%s\n", get_error_message().c_str());
        }
    }

    //////////////////////////////////////////////////////////////////////
    // set window styles transparent, borderless etc

    void set_window_options(HWND hwnd)
    {
        uint32 window_style = border_window_style;
        uint32 window_ex_style = WS_EX_APPWINDOW;
        wchar const *title = window_title.c_str();
        HWND topmost = HWND_NOTOPMOST;

        if(settings.window_borderless) {
            title = null;
            window_style = borderless_window_style;
            window_ex_style = WS_EX_TOOLWINDOW;
        }

        if(settings.window_transparency < 255) {
            window_ex_style |= WS_EX_LAYERED;
        }

        if(settings.window_topmost) {
            topmost = HWND_TOPMOST;
        }

        // fix win32 flickering by hiding window while certain changes are happening
        uint32 old_ex_style = (GetWindowLong(hwnd, GWL_EXSTYLE) & window_ex_style) & ~WS_EX_LAYERED;
        uint32 new_ex_style = window_ex_style & ~WS_EX_LAYERED;
        uint32 old_style = GetWindowLong(hwnd, GWL_STYLE) & window_style;
        if((old_ex_style != new_ex_style || old_style != window_style) && window_style == borderless_window_style) {
            ShowWindow(hwnd, SW_HIDE);
        }

        rect r;
        GetClientRect(hwnd, &r);
        MapWindowRect(hwnd, null, &r);
        SetMenu(hwnd, null);
        SetWindowLong(hwnd, GWL_STYLE, window_style);
        SetWindowLong(hwnd, GWL_EXSTYLE, window_ex_style);
        AdjustWindowRectEx(&r, window_style, false, window_ex_style);

        if(settings.window_transparency < 255) {
            SetLayeredWindowAttributes(hwnd, 0, settings.window_transparency, LWA_ALPHA);
        }

        SetWindowPos(
            hwnd, topmost, r.left, r.top, r.width(), r.height(), SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_DRAWFRAME);
    }

    //////////////////////////////////////////////////////////////////////

    void init_interface_dialog(HWND hwnd)
    {
        HWND list_view = GetDlgItem(hwnd, IDC_LIST_ADAPTERS);

        auto style = LVS_EX_FULLROWSELECT;
        ListView_SetExtendedListViewStyleEx(list_view, style, style);

        insert_listview_column(list_view, L"Adapter", 0, 300);
        insert_listview_column(list_view, L"Type", 1, 120);
        insert_listview_column(list_view, L"Connected", 2, 70);
        insert_listview_column(list_view, L"Physical", 3, 70);

        for(int i = 0; i < (int)all_adapters->NumEntries; i++) {
            MIB_IF_ROW2 &row = all_adapters->Table[i];

            LVITEM item;
            item.iItem = 0;
            item.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            item.stateMask = 0;
            item.iSubItem = 0;
            item.state = 0;
            item.lParam = reinterpret_cast<LPARAM>(&row);
            item.pszText = row.Description; // row.Alias;

            // add it to the list
            ListView_InsertItem(list_view, &item);
            auto type_name = get_interface_type_name(row.Type);
            wchar const *physical = L"Physical";
            wchar const *connected = L"Yes";
            if(!row.InterfaceAndOperStatusFlags.HardwareInterface) {
                physical = L"Virtual";
            }
            if(row.InterfaceAndOperStatusFlags.NotMediaConnected) {
                connected = L"No";
            }
            ListView_SetItemText(list_view, 0, 1, const_cast<wchar *>(type_name));
            ListView_SetItemText(list_view, 0, 2, const_cast<wchar *>(connected));
            ListView_SetItemText(list_view, 0, 3, const_cast<wchar *>(physical));
        }
    }

    //////////////////////////////////////////////////////////////////////

    intptr CALLBACK interface_dlgproc(HWND hdlg, UINT message, WPARAM wparam, LPARAM lparam)
    {
        UNREFERENCED_PARAMETER(lparam);

        switch(message) {

        case WM_INITDIALOG: {
            init_interface_dialog(hdlg);
            return 1;
        }

        case WM_CLOSE:
            EndDialog(hdlg, IDCANCEL);
            return 1;

        // dialog going away - if adapter changed, restart the graph
        case WM_DESTROY:
            if(adapter_valid && adapter_changed) {
                net_stats.start(1, &current_adapter, settings.bars_per_second);
                renderer._start_time = time_now();
                renderer._prev_time = renderer._start_time;
            }
            break;

        // a row in the listview was selected
        case WM_NOTIFY: {
            auto nmlv = reinterpret_cast<LPNMLISTVIEW>(lparam);
            switch(nmlv->hdr.code) {
            case LVN_ITEMCHANGED: {
                if((nmlv->uNewState & LVIS_FOCUSED) != 0) {
                    MIB_IF_ROW2 *selection = reinterpret_cast<MIB_IF_ROW2 *>(nmlv->lParam);
                    if(selection != null && !IsEqualGUID(current_adapter.InterfaceGuid, selection->InterfaceGuid)) {
                        current_adapter = *selection; // memcpy
                        adapter_changed = true;
                        adapter_valid = true;
                    }
                }
            } break;
            }
        } break;

        case WM_COMMAND:
            switch(LOWORD(wparam)) {
            case IDCANCEL:
            case IDOK:
                EndDialog(hdlg, LOWORD(wparam));
                return 1;
            }
            break;
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    intptr CALLBACK choose_color_dlgproc(HWND hdlg, UINT message, WPARAM wparam, LPARAM lparam)
    {
        switch(message) {

        case WM_INITDIALOG: {
            SetWindowLong(hdlg, GWL_USERDATA, lparam); // save ptr to color
            D2D1_COLOR_F *c = reinterpret_cast<D2D1_COLOR_F *>(lparam);
            setup_trackbar(hdlg, IDC_SLIDER_R, 0, 255, static_cast<int>(c->r * 255), IDC_STATIC_R);
            setup_trackbar(hdlg, IDC_SLIDER_G, 0, 255, static_cast<int>(c->g * 255), IDC_STATIC_G);
            setup_trackbar(hdlg, IDC_SLIDER_B, 0, 255, static_cast<int>(c->b * 255), IDC_STATIC_B);
            setup_trackbar(hdlg, IDC_SLIDER_A, 0, 255, static_cast<int>(c->a * 255), IDC_STATIC_A);
            return 1;
        } break;

        case WM_NOTIFY: {
            LPNMHDR h = reinterpret_cast<LPNMHDR>(lparam);
            int r, g, b, a;
            D2D1_COLOR_F *c = reinterpret_cast<D2D1_COLOR_F *>(GetWindowLong(hdlg, GWL_USERDATA));
            switch(LOWORD(wparam)) {

            case IDC_SLIDER_R:
                report_trackbar(hdlg, IDC_SLIDER_R, r, IDC_STATIC_R);
                c->r = r / 255.0f;
                renderer.on_settings_changed();
                break;

            case IDC_SLIDER_G:
                report_trackbar(hdlg, IDC_SLIDER_G, g, IDC_STATIC_G);
                c->g = g / 255.0f;
                renderer.on_settings_changed();
                break;

            case IDC_SLIDER_B:
                report_trackbar(hdlg, IDC_SLIDER_B, b, IDC_STATIC_B);
                c->b = b / 255.0f;
                renderer.on_settings_changed();
                break;

            case IDC_SLIDER_A:
                report_trackbar(hdlg, IDC_SLIDER_A, a, IDC_STATIC_A);
                c->a = a / 255.0f;
                renderer.on_settings_changed();
                break;
            }
            return 1;
        }

        case WM_CLOSE:
            EndDialog(hdlg, IDCANCEL);
            return 1;

        case WM_COMMAND:
            EndDialog(hdlg, LOWORD(wparam));
            return 1;
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    void choose_color(HWND hdlg, D2D1_COLOR_F &color)
    {
        auto old_color = color;
        auto lp = reinterpret_cast<LPARAM>(&color);
        auto rsrc = MAKEINTRESOURCE(IDD_CHOOSECOLORDIALOG);
        if(DialogBoxParam(instance_handle, rsrc, hdlg, &choose_color_dlgproc, lp) != IDOK) {
            color = old_color;
            renderer.on_settings_changed();
        }
    }

    //////////////////////////////////////////////////////////////////////

    void setup_options_dlg(HWND hdlg)
    {
        old_settings = settings;
        wstring build_timestamp = wide_string_from_string(__DATE__ " " __TIME__);
        wstring version_string = get_build_version();
        uint num_chars = GetWindowTextLengthW(GetDlgItem(hdlg, IDC_STATIC_VERSION)) + 1;
        wstring text(num_chars, 0);
        GetDlgItemTextW(hdlg, IDC_STATIC_VERSION, &text[0], num_chars);
        replace_str(text, L"{{V}}", version_string);
        replace_str(text, L"{{B}}", build_timestamp);
        SetDlgItemTextW(hdlg, IDC_STATIC_VERSION, text.c_str());
        Button_SetCheck(GetDlgItem(hdlg, IDC_CHECK_BARGAP), settings.bar_gap);
        Button_SetCheck(GetDlgItem(hdlg, IDC_CHECK_WINDOWONTOP), settings.window_topmost);
        int bps = static_cast<int>(settings.bars_per_second);
        int bar_width = static_cast<int>(settings.bar_width);
        setup_trackbar(hdlg, IDC_SLIDER_SPEED, 1, 60, bps, IDC_STATIC_SPEED);
        setup_trackbar(hdlg, IDC_SLIDER_BARWIDTH, 2, 50, bar_width, IDC_STATIC_SPEED);
    }

    //////////////////////////////////////////////////////////////////////

    intptr CALLBACK options_dlgproc(HWND hdlg, UINT message, WPARAM wparam, LPARAM lparam)
    {
        switch(message) {
        case WM_INITDIALOG: {
            setup_options_dlg(hdlg);
            return 1;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 0);
            return 1;

        case WM_NOTIFY: {
            LPNMHDR h = reinterpret_cast<LPNMHDR>(lparam);
            switch(LOWORD(wparam)) {

            case IDC_SLIDER_SPEED: {
                int bps;
                report_trackbar(hdlg, IDC_SLIDER_SPEED, bps, IDC_STATIC_SPEED);
                settings.bars_per_second = static_cast<double>(bps);
                net_stats._sample_time = settings.bars_per_second;
            } break;

            case IDC_SLIDER_BARWIDTH: {
                int bw;
                report_trackbar(hdlg, IDC_SLIDER_BARWIDTH, bw, IDC_STATIC_BARWIDTH);
                settings.bar_width = static_cast<uint32>(bw);
            } break;
            }
            return 1;
        }

        case WM_COMMAND:
            switch(LOWORD(wparam)) {

            case IDC_CHECK_BARGAP:
                settings.bar_gap = Button_GetCheck(GetDlgItem(hdlg, IDC_CHECK_BARGAP));
                break;

            case IDC_CHECK_WINDOWONTOP:
                settings.window_topmost = Button_GetCheck(GetDlgItem(hdlg, IDC_CHECK_WINDOWONTOP));
                set_window_options(main_hwnd);
                break;

            case IDC_BUTTON_INCOL:
                choose_color(hdlg, settings.in_bar_color);
                break;

            case IDC_BUTTON_OUTCOL:
                choose_color(hdlg, settings.out_bar_color);
                break;

            case IDC_BUTTON_BGCOL:
                choose_color(hdlg, settings.bg_color);
                break;

            case IDC_BUTTON_LINECOL:
                choose_color(hdlg, settings.line_color);
                break;

            case IDC_BUTTON_TXTCOL:
                choose_color(hdlg, settings.text_color);
                break;

            case IDCANCEL:
                settings = old_settings;
                set_window_options(main_hwnd);
                EndDialog(hdlg, LOWORD(wparam));
                break;

            case IDOK:
                set_window_options(main_hwnd);
                EndDialog(hdlg, LOWORD(wparam));
                break;
            }
            return 1;
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    intptr CALLBACK help_dlgproc(HWND hdlg, UINT message, WPARAM wparam, LPARAM lparam)
    {
        UNREFERENCED_PARAMETER(lparam);

        switch(message) {

        case WM_INITDIALOG: {
            wstring help_text = load_resource_string(IDS_STRINGHELP);
            HWND help_text_box = GetDlgItem(hdlg, IDC_EDITHELPTEXT);
            SendMessage(help_text_box, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), false);
            SetDlgItemTextW(hdlg, IDC_EDITHELPTEXT, help_text.c_str());
            return 1;
        }

        case WM_COMMAND:
            EndDialog(hdlg, LOWORD(wparam));
            return 1;
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    DWORD WINAPI options_dlg_thread_fn(void *param)
    {
        return DialogBox(instance_handle, MAKEINTRESOURCE(IDD_OPTIONSDIALOG), main_hwnd, options_dlgproc);
    }

    //////////////////////////////////////////////////////////////////////

    DWORD WINAPI notify_menu_function(HWND w)
    {
        POINT cp;
        GetCursorPos(&cp);
        SetForegroundWindow(w);
        if(TrackPopupMenu(popup_menu, 0, cp.x, cp.y, 0, w, null) == 0) {
            LOG_Error(L"Huh? Can't TrackPopupMenu");
        }
        PostMessage(w, WM_NULL, 0, 0);
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK main_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
    {
        LOG_Context("WNDPROC");

        switch(message) {

        // menu handler
        case WM_COMMAND:

            switch(LOWORD(wparam)) {

            case ID_FILE_OPTIONS: {
                CloseHandle(CreateThread(null, 0, &options_dlg_thread_fn, null, 0, null));
            } break;
            case ID_FILE_ADAPTER:
                DialogBox(instance_handle, MAKEINTRESOURCE(IDD_INTERFACEDIALOG), hwnd, interface_dlgproc);
                break;
            case ID_FILE_HELP:
                DialogBox(instance_handle, MAKEINTRESOURCE(IDD_HELPDIALOG), hwnd, help_dlgproc);
                break;
            case IDM_EXIT:
                DestroyWindow(hwnd);
                break;
            default:
                return DefWindowProc(hwnd, message, wparam, lparam);
            }
            break;

        // moved across monitors with different DPI
        case WM_DPICHANGED: {
            rect *r = reinterpret_cast<rect *>(lparam);
            r->clamp_min_size(300, 200);
            SetWindowPos(hwnd, null, r->left, r->top, r->width(), r->height(), SWP_NOZORDER | SWP_NOACTIVATE);
        } break;

        // clicked the notification icon, pop up a menu
        case WMAPP_NOTIFYCALLBACK: {
            switch(LOWORD(lparam)) {
            case NIN_SELECT:
            case WM_RBUTTONUP:
                notify_menu_function(hwnd);
                break;
            case WM_LBUTTONDOWN:
                SetForegroundWindow(hwnd);
                break;
            }
        } break;

        // double click in the graph (in borderless mode) or the caption bar (in bordered mode) toggles borderless
        case WM_NCLBUTTONDBLCLK:
            if(wparam == HTCAPTION) {
                settings.window_borderless = 1 - settings.window_borderless;
                set_window_options(hwnd);
            }
            break;

        // keyboard
        case WM_CHAR:
            switch(wparam) {
            case ' ':
                settings.window_borderless = !settings.window_borderless;
                set_window_options(hwnd);
            case ']':
                settings.window_transparency = (std::min)(255u, settings.window_transparency + 8);
                set_window_options(hwnd);
                break;
            case '[':
                settings.window_transparency = (std::max)(64u, settings.window_transparency - 8);
                set_window_options(hwnd);
                break;
            case 't':
                settings.window_topmost = !settings.window_topmost;
                set_window_options(hwnd);
                break;
            case 'r': {
                wchar filename_buffer[MAX_PATH] = L"network_monitor_capture.csv";
                OPENFILENAMEW ofn;
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(OPENFILENAMEW);
                ofn.hwndOwner = hwnd;
                ofn.hInstance = instance_handle;
                ofn.lpstrFilter = L"CSV Files\0*.csv\0\0";
                ofn.lpstrFile = filename_buffer;
                ofn.nMaxFile = _countof(filename_buffer);
                ofn.lpstrTitle = L"Save NetworkMonitor bandwidth capture";
                ofn.Flags = OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                ofn.lpstrDefExt = L"csv";
                if(GetSaveFileNameW(&ofn) != 0) {
                    LOG_Info(L"Saving capture to %s", ofn.lpstrFile);
                    renderer._recording = net_stats.start_recording(ofn.lpstrFile);
                }
                else {
                    LOG_Error(WindowsError(CommDlgExtendedError(), L"Can't SaveFilenameDialog").c_str());
                }
            } break;
            case 's':
                renderer._recording = false;
                net_stats.stop_recording();
                break;
            case 27:
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
            }
            break;

        // right click on the graph (in borderless) or caption (bordered) pops up the menu
        case WM_NCRBUTTONDOWN: {
            if(wparam == HTCAPTION) {
                TrackPopupMenu(popup_menu, 0, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), 0, hwnd, null);
            }
        } break;

        // right click on the graph pops up the menu
        case WM_RBUTTONDOWN: {
            POINT p{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            ClientToScreen(hwnd, &p);
            TrackPopupMenu(popup_menu, 0, p.x, p.y, 0, hwnd, null);
        } break;

        // window admin
        case WM_SIZE:
            renderer.resize(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            renderer.render();
            break;

        case WM_SIZING: {
            rect *r = reinterpret_cast<rect *>(lparam);
            r->clamp_min_size(300, 200);
            renderer.resize(r->width(), r->height());
            renderer.render();
        } break;

        case WM_NCHITTEST:
            return nc_hit_test(hwnd, message, wparam, lparam);

        // No need to erase background or paint with d3d render hammering away
        case WM_ERASEBKGND:
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
        } break;

        case WM_DESTROY: {
            rect r;
            GetClientRect(hwnd, &r);
            MapWindowRect(hwnd, null, &r);
            settings.window_x = r.left;
            settings.window_y = r.top;
            settings.window_w = r.width();
            settings.window_h = r.height();
            delete_notification_icon(hwnd);
            net_stats.stop();
            PostQuitMessage(0);
        } break;

        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    void free_all_adapters()
    {
        if(all_adapters != null) {
            FreeMibTable(all_adapters);
            all_adapters = null;
        }
    }

    //////////////////////////////////////////////////////////////////////

    void choose_an_adapter()
    {
        free_all_adapters();

        if(FAILED(GetIfTable2(&all_adapters))) {
            trace(L"Error calling GetIfTable2: %s\n", get_error_message().c_str());
            return;
        }

        MIB_IF_ROW2 *candidate = null;

        // pick a random physical, connected adapter (or the one in the settings)
        for(uint i = 0; i < all_adapters->NumEntries; ++i) {
            MIB_IF_ROW2 &a = all_adapters->Table[i];
            if(IsEqualGUID(settings.network_adapter_guid, a.InterfaceGuid)) {
                candidate = &a;
                break;
            }
            // connected, physical interface is a good candidate
            if(a.InterfaceAndOperStatusFlags.HardwareInterface && !a.InterfaceAndOperStatusFlags.NotMediaConnected) {
                candidate = &a;
            }
        }

        if(candidate != null) {
            current_adapter = *candidate;
            adapter_changed = true;
            adapter_valid = true;
        }
    }

} // namespace

//////////////////////////////////////////////////////////////////////

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR command_line, int cmd_show)
{
    UNREFERENCED_PARAMETER(prev_instance);
    UNREFERENCED_PARAMETER(command_line);

    instance_handle = instance;

    Log_SetLevel(Verbose);

    window_title = load_resource_string(IDS_APP_TITLE);
    window_class_name = load_resource_string(IDS_WINDOW_CLASS);
    window_menu = LoadMenu(instance_handle, MAKEINTRESOURCEW(IDC_NETWORKMONITOR));
    popup_menu = GetSubMenu(window_menu, 0);

    HWND already_running_hwnd = FindWindow(window_class_name.c_str(), window_title.c_str());
    if(already_running_hwnd != null) {
        ShowWindow(already_running_hwnd, SW_RESTORE);
        BringWindowToTop(already_running_hwnd);
        SetForegroundWindow(already_running_hwnd);
        return 0;
    }

    settings.load();

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = main_wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_MAINICON));
    wcex.hCursor = LoadCursor(null, IDC_ARROW);
    wcex.hbrBackground = null;
    wcex.lpszMenuName = null;
    wcex.lpszClassName = window_class_name.c_str();
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    if(RegisterClassExW(&wcex) == 0) {
        trace(L"RegisterClassExW failed: %s\n", get_error_message().c_str());
    }

    rect window_rect;

    window_rect.left = settings.window_x;
    window_rect.top = settings.window_y;
    window_rect.right = settings.window_x + settings.window_w;
    window_rect.bottom = settings.window_y + settings.window_h;

    uint32 window_styles = settings.window_borderless ? borderless_window_style : border_window_style;
    uint32 window_ex_styles = settings.window_borderless ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW;

    AdjustWindowRectEx(&window_rect, window_styles, false, window_ex_styles);

    main_hwnd = CreateWindowExW(0,
        window_class_name.c_str(),
        window_title.c_str(),
        window_styles,
        window_rect.left,
        window_rect.top,
        window_rect.width(),
        window_rect.height(),
        null,
        null,
        instance,
        null);

    if(main_hwnd == null) {
        trace(L"CreateWindowExW failed: %s\n", get_error_message().c_str());
        return 1;
    }

    // sets the window styles from settings and shows the window
    set_window_options(main_hwnd);

    add_notification_icon(main_hwnd);

    choose_an_adapter();

    if(adapter_valid) {
        net_stats.start(1, &current_adapter, settings.bars_per_second);
        adapter_changed = false;
    }
    else {
        DialogBox(instance_handle, MAKEINTRESOURCE(IDD_INTERFACEDIALOG), main_hwnd, interface_dlgproc);
    }

    if(FAILED(renderer.init(main_hwnd))) {
        trace(L"init_d3d failed: %s\n", get_error_message().c_str());
        return 1;
    }

    bool quit = false;
    while(!quit) {

        renderer.render();

        MSG msg;
        while(PeekMessage(&msg, null, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                quit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    renderer.release();

    if(adapter_valid) {
        settings.network_adapter_guid = current_adapter.InterfaceGuid;
    }
    settings.save();
    free_all_adapters();

    return 0;
}
