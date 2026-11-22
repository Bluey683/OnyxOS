#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <wrl/client.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <windows.ui.core.h>

namespace onyx { namespace graphics {

// Native CoreWindow renderer. This is the target display path for the
// XAML-free Xbox host; it owns a DXGI CoreWindow swap chain and a guest texture.
class CoreWindowD3DRenderer final
{
public:
    bool Initialize(Windows::UI::Core::CoreWindow^ window,
                    std::uint32_t width, std::uint32_t height,
                    std::wstring& error);
    bool Resize(std::uint32_t width, std::uint32_t height, std::wstring& error);
    bool Present(const std::vector<std::uint32_t>& bgra,
                 int width, int height, std::wstring& error);

private:
    bool CreateGuestTexture(int width, int height, std::wstring& error);
    bool CreateBackBuffer(std::wstring& error);

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_target;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_guestTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_guestView;
    int m_guestWidth{0};
    int m_guestHeight{0};
};

}}
