//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

struct dx_renderer
{
    dx_renderer() = default;

    HRESULT init(HWND hwnd);
    HRESULT resize(int width, int height);
    void release();
    HRESULT render();

    virtual HRESULT on_create() = 0;
    virtual void on_render(int width, int height) = 0;
    virtual HRESULT on_settings_changed() = 0;
    virtual void on_release() = 0;

    double _start_time = 0; // startup timestamp (local epoch)
    double _prev_time = 0;  // timestamp prev frame (relative to start_time)
    double _wall_time = 0;  // timestamp this frame (relative to start_time)
    double _delta_time = 0; // delta this frame to last frame

    D3D_FEATURE_LEVEL _feature_level = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device *_d3d_device = null;
    ID3D11DeviceContext *_d3d_context = null;
    IDXGISwapChain *_swap_chain = null;
    ID3D11RenderTargetView *_render_target_view = null;
    ID2D1Factory *_d2d_factory = null;
    ID2D1RenderTarget *_d2d_render_target = null;
    IDWriteFactory *_directwrite_factory = null;

    HRESULT create_render_targets(int width, int height);
    HRESULT create_swapchain(IDXGIFactory *dxgiFactory, HWND hwnd, int width, int height);
    HRESULT get_dxgi_factory(ID3D11Device *d3d_device, IDXGIFactory1 **dxgi_factory_ptr);
};
