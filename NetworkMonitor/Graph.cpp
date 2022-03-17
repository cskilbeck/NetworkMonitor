//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

namespace
{
    //////////////////////////////////////////////////////////////////////
    // get a useful Y scale for the graph

    double get_graph_scale_units(double m)
    {
        double scale = 0.001;
        double seek = m / 10;
        while(scale < seek) {
            scale *= 10;
        }
        return scale;
    }

    //////////////////////////////////////////////////////////////////////
    // get where the top line of the y axis should be

    double get_nearest_upper(double m, double scale)
    {
        return floor(m / scale + 1) * scale;
    }

    //////////////////////////////////////////////////////////////////////
    // is it in Kbps, Mbps or Gbps

    std::pair<wchar const *, double> get_scale_name(double m)
    {
        if(m < 1) {
            return { L"K", 1000 };
        }
        else if(m < 1000) {
            return { L"M", 1 };
        }
        else {
            return { L"G", 0.001 };
        }
    }
}

//////////////////////////////////////////////////////////////////////

graph_renderer::graph_renderer(network_stats &net_stats)
    : dx_renderer()
    , _net_stats(net_stats)
    , _left_margin(10)
    , _right_margin(10)
    , _top_margin(10)
    , _bottom_margin(10)
    , _in_graph(bandwidth_in, _net_stats, *this)
    , _out_graph(bandwidth_out, _net_stats, *this)
    , _new_settings(false)
    , _recording(false)
{
}

//////////////////////////////////////////////////////////////////////

HRESULT graph_renderer::on_create()
{
    D2D1_COLOR_F recording_color{ 0.666f, 0, 0, 1 };
    D2D1_COLOR_F black_color{ 0.15f, 0.15f, 0.15f, 1 };
    HR(_d2d_render_target->CreateSolidColorBrush(recording_color, &_recording_brush));
    HR(_d2d_render_target->CreateSolidColorBrush(black_color, &_black_brush));

    HR(_in_graph.init(settings.in_bar_color));
    HR(_out_graph.init(settings.out_bar_color));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////

void graph_renderer::on_release()
{
    _in_graph.release();
    _out_graph.release();
}

//////////////////////////////////////////////////////////////////////

void graph_renderer::on_render(int width, int height)
{
    int bar_index;
    {
        std::lock_guard<std::mutex> net_stats_guard(_net_stats._mutex);
        bar_index = _net_stats.get_current_bar_index();

        if(_new_settings) {
            _in_graph.release();
            _out_graph.release();
            _in_graph.init(settings.in_bar_color);
            _out_graph.init(settings.out_bar_color);
            _new_settings = false;
        }
    }

    int hh = height / 2;
    int m2 = static_cast<int>(_top_margin / 2);
    _in_graph.draw(0, 0, width, hh + m2, bar_index);
    _out_graph.draw(0, hh - m2, width, hh + m2, bar_index);

    if(_recording) {
        _d2d_render_target->SetDpi(0, 0);
        float iw = 14;
        D2D1_ELLIPSE ellipse{ { width - iw * 2.5f, iw * 2.5f }, iw, iw };
        _d2d_render_target->SetTransform(D2D1::Matrix3x2F::Identity());
        _d2d_render_target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        _d2d_render_target->FillEllipse(ellipse, _recording_brush);
        _d2d_render_target->DrawEllipse(ellipse, _black_brush, 2);
    }
}

//////////////////////////////////////////////////////////////////////

graph::graph(net_sample_type sample_type, network_stats &net_stats, graph_renderer &gr)
    : _net_stats(net_stats)
    , _graph_renderer(gr)
    , _sample_type(sample_type)
{
}

//////////////////////////////////////////////////////////////////////

HRESULT graph_renderer::on_settings_changed()
{
    _new_settings = true;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT graph::init(D2D1_COLOR_F color)
{
    ID2D1RenderTarget *const rt = _graph_renderer._d2d_render_target;

    // brushes for bars, lines and text
    HR(rt->CreateSolidColorBrush(color, &_bar_brush));
    HR(rt->CreateSolidColorBrush(settings.text_color, &_text_brush));
    HR(rt->CreateSolidColorBrush(settings.line_color, &_line_brush));

    // create font (TextFormat thing)
    auto weight = DWRITE_FONT_WEIGHT_NORMAL;
    auto style = DWRITE_FONT_STYLE_NORMAL;
    auto stretch = DWRITE_FONT_STRETCH_NORMAL;
    auto size = 10.0f;
    HR(_graph_renderer._directwrite_factory->CreateTextFormat(
        L"Arial", null, weight, style, stretch, size, L"en-us", &_text_format));
    auto spacing_method = DWRITE_LINE_SPACING_METHOD_DEFAULT;

    // measure the font height
    std::wstring str{ L"1000M" };
    IDWriteTextLayout *layout = null;
    uint32 l = (uint32)str.size();
    float w = 10000;
    float h = 10000;
    HR(_graph_renderer._directwrite_factory->CreateTextLayout(str.c_str(), l, _text_format, w, h, &layout));
    defer(layout->Release());
    DWRITE_TEXT_METRICS metrics;
    HR(layout->GetMetrics(&metrics));
    _line_spacing = metrics.height * 1.1f;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////

void graph::release()
{
    com_release(_text_brush);
    com_release(_line_brush);
    com_release(_text_format);
    com_release(_bar_brush);
}

//////////////////////////////////////////////////////////////////////

void graph::draw(int x, int y, int width, int height, int bar_index)
{
    float l = _graph_renderer._left_margin + x;
    float t = _graph_renderer._top_margin + y;

    float w = static_cast<float>(width - (_graph_renderer._left_margin + _graph_renderer._right_margin));
    float h = static_cast<float>(height - (_graph_renderer._top_margin + _graph_renderer._bottom_margin));

    float r = l + w;
    float b = t + h;

    ID2D1RenderTarget *const rt = _graph_renderer._d2d_render_target;

    auto text = [=](std::wstring const &str, float x, float y) -> HRESULT {
        IDWriteTextLayout *layout = null;
        uint32 l = (uint32)str.size();
        HR(_graph_renderer._directwrite_factory->CreateTextLayout(str.c_str(), l, _text_format, w, h, &layout));
        defer(layout->Release());
        rt->DrawTextLayout({ x, y }, layout, _text_brush);
        return S_OK;
    };

    // border
    rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    rt->SetTransform(D2D1::Matrix3x2F::Identity());
    rt->DrawLine({ l - 1, t }, { r, t }, _line_brush);
    rt->DrawLine({ l, t }, { l, b }, _line_brush);
    rt->DrawLine({ r, t }, { r, b }, _line_brush);
    rt->DrawLine({ l, b }, { r - 1, b }, _line_brush);

    // graph
    auto clip_rect = D2D1_RECT_F{ l, t, l + w - 1, t + h - 1 };
    rt->PushAxisAlignedClip(clip_rect, D2D1_ANTIALIAS_MODE_ALIASED);
    defer(rt->PopAxisAlignedClip());

    rt->SetTransform(D2D1::Matrix3x2F::Translation(l, t));
    rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    if(bar_index > 0) {

        _max_bandwidth = (std::max)(_max_bandwidth, settings.min_scale_mbps);
        _max_bandwidth = (std::min)(_max_bandwidth, settings.max_scale_mbps);

        double vertical_scale = 0.001;
        if(_max_bandwidth > 0) {
            vertical_scale = (h - 10) / _max_bandwidth;
        }

        double seconds_per_bar = 1.0 / settings.bars_per_second;
        int bar_total_width = settings.bar_width + settings.bar_gap;

        int bars_drawn = 0;
        double max_drawn = 0;
        double bar_start_time;

        while(true) {

            // scan backwards through the bars (from new to old, right to left)
            bar_index -= 1;
            if(bar_index < 0) {
                bar_index += _net_stats._num_bars;
            }
            net_sample &sample = _net_stats._bars[bar_index];

            // grotty hack to cheat the aliasing due to timing noise
            if(bars_drawn == 0) {
                bar_start_time = sample._start_time;
            }
            else {
                bar_start_time -= seconds_per_bar;
            }

            // draw the bar
            double bar_time = _graph_renderer._wall_time - bar_start_time;
            double bar_x = bar_time / seconds_per_bar * bar_total_width - bar_total_width;
            double bandwidth = sample.get_average(_sample_type);

            float x = static_cast<float>(bar_x);
            float y = static_cast<float>(bandwidth * vertical_scale);
            D2D1_RECT_F rect = D2D1::RectF(w - x, h - 1, w - x + settings.bar_width, h - y - 1);
            rt->FillRectangle(&rect, _bar_brush);

            // keep track of tallest bar for scaling
            max_drawn = (std::max)(bandwidth, max_drawn);

            // done enough? (drawn them all or gone off the left edge of the window)
            bars_drawn += 1;
            if(x > w || bars_drawn >= _net_stats._current_bar) {
                break;
            }
        }

        // scale
        double bandwidth_delta = max_drawn - _max_bandwidth;

        if(bandwidth_delta < 0) {
            bandwidth_delta *= settings.scale_down_rate;
        }
        else if(bandwidth_delta > 0) {
            bandwidth_delta *= settings.scale_up_rate;
        }
        _max_bandwidth += bandwidth_delta;

        double y_axis_scale = 0.001;

        while(y_axis_scale * vertical_scale > h / 4) {
            y_axis_scale /= 10;
        }
        while(y_axis_scale * vertical_scale < _line_spacing + 4) {
            y_axis_scale *= 10;
        }

        double y_axis_show = y_axis_scale;
        double y_axis_mult = 1;
        while(y_axis_show * y_axis_mult > 100) {
            y_axis_mult /= 10;
        }
        while(y_axis_show * y_axis_mult < 0.01) {
            y_axis_mult *= 10;
        }

        auto scale_name = get_scale_name(y_axis_show);
        wchar const *y_axis_label_format = L"%-0.0f%s";
        if(y_axis_show < 0.1) {
            y_axis_label_format = L"%-0.1f%s";
        }
        else if(y_axis_show < 1) {
            y_axis_label_format = L"%-0.0f%s";
        }

        // axis
        rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
        rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

        double axis_y = 0;
        float old_y = 0;
        while(true) {
            float ay = (float)ceil(axis_y * vertical_scale);
            if(ay > h) {
                break;
            }
            float y = (float)(h - ay);
            rt->DrawLine({ 0, y }, { w, y }, _line_brush);
            text(format(y_axis_label_format, axis_y * scale_name.second * y_axis_mult, scale_name.first), 0, y);
            old_y = y;
            axis_y += y_axis_scale;
        }
    }
}
