//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

struct graph_renderer;

struct graph
{
    graph(net_sample_type sample_type, network_stats &net_stats, graph_renderer &gr);

    HRESULT init(D2D1_COLOR_F color);
    void release();

    void draw(int x, int y, int width, int height, int bar_index);

    network_stats &_net_stats;
    graph_renderer &_graph_renderer;

    net_sample_type _sample_type = bandwidth_in;

    double _max_bandwidth = 0;

    float _line_spacing;
    float _baseline;

    ID2D1SolidColorBrush *_bar_brush = null;
    ID2D1SolidColorBrush *_text_brush = null;
    ID2D1SolidColorBrush *_line_brush = null;
    IDWriteTextFormat *_text_format = null;
};

//////////////////////////////////////////////////////////////////////

struct graph_renderer : dx_renderer
{
    graph_renderer(network_stats &net_stats);

    HRESULT on_create() override;
    void on_release() override;
    HRESULT on_settings_changed() override;
    void on_render(int width, int height) override;

    network_stats &_net_stats;

    float _left_margin;
    float _right_margin;
    float _top_margin;
    float _bottom_margin;

    graph _in_graph;
    graph _out_graph;

    bool _new_settings;
    bool _recording;

    ID2D1SolidColorBrush *_recording_brush = null;
    ID2D1SolidColorBrush *_black_brush = null;
};
