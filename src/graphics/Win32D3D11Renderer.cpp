#include "Win32D3D11Renderer.h"

#include <winapifamily.h>

namespace VMwareXbox
{
    Win32D3D11Renderer::~Win32D3D11Renderer() noexcept
    {
        Shutdown();
    }

    HRESULT Win32D3D11Renderer::Initialize(HWND window) noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (!window || m_swapChain)
            return E_INVALIDARG;

        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        const D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };
        D3D_FEATURE_LEVEL selected{};
        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
            levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
            m_device.GetAddressOf(), &selected, m_context.GetAddressOf());

        if (hr == E_INVALIDARG)
        {
            hr = D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                levels + 1, ARRAYSIZE(levels) - 1, D3D11_SDK_VERSION,
                m_device.ReleaseAndGetAddressOf(), &selected,
                m_context.ReleaseAndGetAddressOf());
        }
        if (FAILED(hr))
            return hr;

        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        hr = m_device.As(&dxgiDevice);
        if (FAILED(hr)) return hr;

        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());
        if (FAILED(hr)) return hr;

        Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
        hr = adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf()));
        if (FAILED(hr)) return hr;

        RECT client{};
        if (!GetClientRect(window, &client))
            return HRESULT_FROM_WIN32(GetLastError());

        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = static_cast<UINT>(client.right - client.left);
        desc.Height = static_cast<UINT>(client.bottom - client.top);
        if (!desc.Width) desc.Width = 1;
        if (!desc.Height) desc.Height = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        hr = factory->CreateSwapChainForHwnd(
            m_device.Get(), window, &desc, nullptr, nullptr,
            m_swapChain.GetAddressOf());
        if (FAILED(hr))
        {
            Shutdown();
            return hr;
        }

        factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
        hr = CreateRenderTarget();
        if (FAILED(hr))
            Shutdown();
        return hr;
#else
        (void)window;
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
#endif
    }

    HRESULT Win32D3D11Renderer::CreateRenderTarget() noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(
            0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        if (FAILED(hr)) return hr;

        return m_device->CreateRenderTargetView(
            backBuffer.Get(), nullptr, m_renderTarget.GetAddressOf());
#else
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
#endif
    }

    HRESULT Win32D3D11Renderer::Resize(UINT width, UINT height) noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (!m_swapChain) return E_FAIL;
        if (!width || !height) return S_FALSE;

        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_renderTarget.Reset();

        HRESULT hr = m_swapChain->ResizeBuffers(
            0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr)) return hr;
        return CreateRenderTarget();
#else
        (void)width; (void)height;
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
#endif
    }

    void Win32D3D11Renderer::Render() noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (!m_context || !m_swapChain || !m_renderTarget)
            return;

        const float clearColor[] = { 0.035f, 0.045f, 0.075f, 1.0f };
        m_context->OMSetRenderTargets(
            1, m_renderTarget.GetAddressOf(), nullptr);
        m_context->ClearRenderTargetView(m_renderTarget.Get(), clearColor);
        m_swapChain->Present(1, 0);
#endif
    }

    void Win32D3D11Renderer::Shutdown() noexcept
    {
        if (m_context)
            m_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_renderTarget.Reset();
        m_swapChain.Reset();
        m_context.Reset();
        m_device.Reset();
    }
}
