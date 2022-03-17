//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

HRESULT dx_renderer::create_render_targets(int width, int height)
{
    ID3D11Texture2D *back_buffer = null;
    HR(_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));
    defer(back_buffer->Release());

    HR(_d3d_device->CreateRenderTargetView(back_buffer, null, &_render_target_view));

    IDXGISurface *dxgi_back_buffer = null;
    HR(_swap_chain->GetBuffer(0, IID_PPV_ARGS(&dxgi_back_buffer)));
    defer(dxgi_back_buffer->Release());

    auto pixel_format = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED);
    auto props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, pixel_format, 0, 0);
    HR(_d2d_factory->CreateDxgiSurfaceRenderTarget(dxgi_back_buffer, &props, &_d2d_render_target));

    _d3d_context->OMSetRenderTargets(1, &_render_target_view, NULL);

    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _d3d_context->RSSetViewports(1, &vp);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT dx_renderer::create_swapchain(IDXGIFactory *dxgiFactory, HWND hwnd, int width, int height)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    HR(dxgiFactory->CreateSwapChain(_d3d_device, &sd, &_swap_chain));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT dx_renderer::get_dxgi_factory(ID3D11Device *d3d_device, IDXGIFactory1 **dxgi_factory_ptr)
{
    IDXGIDevice *dxgi_device = null;
    HR(d3d_device->QueryInterface(IID_PPV_ARGS(&dxgi_device)));
    defer(dxgi_device->Release());

    IDXGIAdapter *adapter = null;
    HR(dxgi_device->GetAdapter(&adapter));
    defer(adapter->Release());

    HR(adapter->GetParent(IID_PPV_ARGS(dxgi_factory_ptr)));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT dx_renderer::init(HWND hwnd)
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(hwnd, &rc);
    uint width = rc.right - rc.left;
    uint height = rc.bottom - rc.top;

#ifdef _DEBUG
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    auto driver = D3D_DRIVER_TYPE_HARDWARE;
    auto options = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
    auto dxver = D3D11_SDK_VERSION;

    HR(D3D11CreateDevice(null, driver, null, options, levels, 1, dxver, &_d3d_device, &_feature_level, &_d3d_context));

    IDXGIFactory1 *dxgi_factory = null;
    HR(get_dxgi_factory(_d3d_device, &dxgi_factory));
    defer(dxgi_factory->Release());

    HR(create_swapchain(dxgi_factory, hwnd, width, height));
    dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_d2d_factory));
    HR(create_render_targets(width, height));

    auto guid = __uuidof(IDWriteFactory);
    auto unk_ptr = reinterpret_cast<IUnknown **>(&_directwrite_factory);
    HR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, guid, unk_ptr));

    _start_time = time_now();
    _prev_time = _start_time;

    HR(on_create());

    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT dx_renderer::resize(int width, int height)
{
    if(_swap_chain != null) {
        _d3d_context->OMSetRenderTargets(0, 0, 0);
        com_release(_render_target_view);
        com_release(_d2d_render_target);
        HR(_swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
        HR(create_render_targets(width, height));
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////

void dx_renderer::release()
{
    if(_d3d_context != null) {
        _d3d_context->ClearState();
    }

    on_release();

    com_release(_d2d_factory);
    com_release(_d2d_render_target);
    com_release(_directwrite_factory);
    com_release(_render_target_view);
    com_release(_swap_chain);
    com_release(_d3d_context);
    com_release(_d3d_device);
}

//////////////////////////////////////////////////////////////////////

HRESULT dx_renderer::render()
{
    if(_d3d_device != null) {

        _wall_time = time_now() - _start_time;
        _delta_time = _wall_time - _prev_time;
        _prev_time = _wall_time;

        _d3d_context->ClearRenderTargetView(_render_target_view, reinterpret_cast<float *>(&settings.bg_color));

        DXGI_SWAP_CHAIN_DESC swap_chain_desc;
        HR(_swap_chain->GetDesc(&swap_chain_desc));

        int w = swap_chain_desc.BufferDesc.Width;
        int h = swap_chain_desc.BufferDesc.Height;

        _d2d_render_target->BeginDraw();

        on_render(w, h);

        HR(_d2d_render_target->EndDraw());
        HR(_swap_chain->Present(1, 0));
    }
    return S_OK;
}
