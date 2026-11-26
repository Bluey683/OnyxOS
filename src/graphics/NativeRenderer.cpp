#include "NativeRenderer.h"

#include <d3d11.h>
#include <dxgi1_2.h>

HRESULT NativeRenderer::Initialize(
    winrt::Windows::UI::Core::CoreWindow const& window)
{
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    constexpr D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL selected{};
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        m_device.GetAddressOf(),
        &selected,
        m_context.GetAddressOf());

    if (hr == E_INVALIDARG)
    {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            levels + 1,
            ARRAYSIZE(levels) - 1,
            D3D11_SDK_VERSION,
            m_device.ReleaseAndGetAddressOf(),
            &selected,
            m_context.ReleaseAndGetAddressOf());
    }

    if (FAILED(hr))
        return hr;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_device.As(&dxgiDevice);
    if (FAILED(hr))
        return hr;

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());
    if (FAILED(hr))
        return hr;

    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    hr = adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = 0;
    desc.Height = 0;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = 0;

    hr = factory->CreateSwapChainForCoreWindow(
        m_device.Get(),
        reinterpret_cast<IUnknown*>(winrt::get_abi(window)),
        &desc,
        nullptr,
        m_swapChain.GetAddressOf());
    if (FAILED(hr))
        return hr;

    return CreateRenderTarget();
}

HRESULT NativeRenderer::CreateRenderTarget()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(
        0,
        IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    return m_device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        m_renderTarget.GetAddressOf());
}

void NativeRenderer::Render()
{
    if (!m_context || !m_swapChain || !m_renderTarget)
        return;

    const float background[] = { 0.035f, 0.045f, 0.075f, 1.0f };
    m_context->OMSetRenderTargets(1, m_renderTarget.GetAddressOf(), nullptr);
    m_context->ClearRenderTargetView(m_renderTarget.Get(), background);
    m_swapChain->Present(1, 0);
}
