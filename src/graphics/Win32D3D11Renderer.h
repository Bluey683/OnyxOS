#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace VMwareXbox
{
    // Desktop Win32 renderer. CreateSwapChainForHwnd is not available to
    // the Xbox/UWP app partition; every entry point is guarded accordingly.
    class Win32D3D11Renderer final
    {
    public:
        Win32D3D11Renderer() noexcept = default;
        ~Win32D3D11Renderer() noexcept;

        Win32D3D11Renderer(const Win32D3D11Renderer&) = delete;
        Win32D3D11Renderer& operator=(const Win32D3D11Renderer&) = delete;

        HRESULT Initialize(HWND window) noexcept;
        HRESULT Resize(UINT width, UINT height) noexcept;
        void Render() noexcept;
        void Shutdown() noexcept;

        bool IsInitialized() const noexcept { return m_swapChain != nullptr; }

    private:
        HRESULT CreateRenderTarget() noexcept;

        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTarget;
    };
}
