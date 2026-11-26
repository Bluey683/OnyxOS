#pragma once

#include <winrt/Windows.UI.Core.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

class NativeRenderer final
{
public:
    HRESULT Initialize(winrt::Windows::UI::Core::CoreWindow const& window);
    void Render();

private:
    HRESULT CreateRenderTarget();

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTarget;
};
