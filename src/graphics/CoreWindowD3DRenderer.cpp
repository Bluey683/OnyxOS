#include "CoreWindowD3DRenderer.h"

#include <d3dcompiler.h>
#include <cstring>
#include <sstream>

using Microsoft::WRL::ComPtr;

namespace onyx { namespace graphics {
namespace {
const char* CoreWindowShader = R"(
Texture2D GuestFramebuffer : register(t0);
SamplerState GuestSampler : register(s0);
struct VOut { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
VOut VSMain(uint id : SV_VertexID) {
    VOut o;
    float2 uv = float2((id << 1) & 2, id & 2);
    o.position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    o.uv = uv;
    return o;
}
float4 PSMain(VOut input) : SV_TARGET {
    return GuestFramebuffer.Sample(GuestSampler, input.uv);
}
)";

std::wstring HrText(const wchar_t* operation, HRESULT hr) {
    std::wstringstream stream;
    stream << operation << L" (HRESULT 0x" << std::hex
           << static_cast<unsigned long>(hr) << L")";
    return stream.str();
}
}

bool CoreWindowD3DRenderer::Initialize(
    Windows::UI::Core::CoreWindow^ window,
    std::uint32_t width, std::uint32_t height,
    std::wstring& error) {
    if (!window || width < 64 || height < 64) {
        error = L"CoreWindow ou dimensão inicial inválida.";
        return false;
    }

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL selected{};
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, flags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
        &m_device, &selected, &m_context);
    if (hr == E_INVALIDARG) {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
            nullptr, flags, levels + 1, ARRAYSIZE(levels) - 1,
            D3D11_SDK_VERSION, &m_device, &selected, &m_context);
    }
    if (FAILED(hr)) {
        error = HrText(L"D3D11CreateDevice falhou", hr);
        return false;
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(hr = m_device.As(&dxgiDevice)) ||
        FAILED(hr = dxgiDevice->GetAdapter(&adapter)) ||
        FAILED(hr = adapter->GetParent(IID_PPV_ARGS(&factory)))) {
        error = HrText(L"Obtenção da fábrica DXGI falhou", hr);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    hr = factory->CreateSwapChainForCoreWindow(
        m_device.Get(), reinterpret_cast<IUnknown*>(window),
        &desc, nullptr, &m_swapChain);
    if (FAILED(hr)) {
        error = HrText(L"CreateSwapChainForCoreWindow falhou", hr);
        return false;
    }

    ComPtr<ID3DBlob> vs, ps, errors;
    hr = D3DCompile(CoreWindowShader, std::strlen(CoreWindowShader),
        nullptr, nullptr, nullptr, "VSMain", "vs_4_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0, &vs, &errors);
    if (FAILED(hr)) {
        error = L"Compilação do vertex shader HLSL falhou.";
        return false;
    }
    hr = D3DCompile(CoreWindowShader, std::strlen(CoreWindowShader),
        nullptr, nullptr, nullptr, "PSMain", "ps_4_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0, &ps, &errors);
    if (FAILED(hr)) {
        error = L"Compilação do pixel shader HLSL falhou.";
        return false;
    }
    if (FAILED(hr = m_device->CreateVertexShader(
            vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &m_vertexShader)) ||
        FAILED(hr = m_device->CreatePixelShader(
            ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &m_pixelShader))) {
        error = HrText(L"Criação dos shaders HLSL falhou", hr);
        return false;
    }

    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(hr = m_device->CreateSamplerState(&sampler, &m_sampler))) {
        error = HrText(L"Criação do sampler falhou", hr);
        return false;
    }
    if (!CreateBackBuffer(error)) return false;

    // Present an intentional startup background immediately. Previously the
    // swap chain was created but never presented until the first guest frame,
    // which made a healthy app look like a frozen black screen during boot.
    const float startupBackground[4] = { 0.035f, 0.055f, 0.085f, 1.0f };
    m_context->OMSetRenderTargets(1, m_target.GetAddressOf(), nullptr);
    m_context->ClearRenderTargetView(m_target.Get(), startupBackground);
    hr = m_swapChain->Present(1, 0);
    if (FAILED(hr)) {
        error = HrText(L"Apresentação da tela inicial falhou", hr);
        return false;
    }
    return true;
}

bool CoreWindowD3DRenderer::CreateBackBuffer(std::wstring& error) {
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        error = HrText(L"Obtenção do backbuffer falhou", hr);
        return false;
    }
    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_target);
    if (FAILED(hr)) {
        error = HrText(L"Criação do render target falhou", hr);
        return false;
    }
    return true;
}

bool CoreWindowD3DRenderer::Resize(
    std::uint32_t width, std::uint32_t height, std::wstring& error) {
    if (!m_swapChain || width < 64 || height < 64) {
        error = L"Swap chain ou dimensão de redimensionamento inválida.";
        return false;
    }
    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_target.Reset();
    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        error = HrText(L"ResizeBuffers falhou", hr);
        return false;
    }
    return CreateBackBuffer(error);
}

bool CoreWindowD3DRenderer::CreateGuestTexture(
    int width, int height, std::wstring& error) {
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        error = L"Dimensão do framebuffer convidado inválida.";
        return false;
    }
    m_guestView.Reset();
    m_guestTexture.Reset();

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_guestTexture);
    if (FAILED(hr)) {
        error = HrText(L"Criação da textura guest falhou", hr);
        return false;
    }
    hr = m_device->CreateShaderResourceView(m_guestTexture.Get(), nullptr, &m_guestView);
    if (FAILED(hr)) {
        error = HrText(L"Criação da ShaderResourceView falhou", hr);
        return false;
    }
    m_guestWidth = width;
    m_guestHeight = height;
    return true;
}

bool CoreWindowD3DRenderer::Present(
    const std::vector<std::uint32_t>& bgra,
    int width, int height, std::wstring& error) {
    if (!m_device || !m_swapChain || !m_target ||
        width <= 0 || height <= 0 ||
        bgra.size() != static_cast<std::size_t>(width) * height) {
        error = L"Framebuffer BGRA inválido ou renderer não inicializado.";
        return false;
    }

    if (width != m_guestWidth || height != m_guestHeight) {
        if (!CreateGuestTexture(width, height, error)) return false;
    }

    m_context->UpdateSubresource(m_guestTexture.Get(), 0, nullptr,
        bgra.data(), static_cast<UINT>(width * sizeof(std::uint32_t)), 0);

    DXGI_SWAP_CHAIN_DESC1 desc{};
    HRESULT hr = m_swapChain->GetDesc1(&desc);
    if (FAILED(hr)) {
        error = HrText(L"Leitura da dimensão da swap chain falhou", hr);
        return false;
    }

    const float clear[4] = { 0.02f, 0.02f, 0.03f, 1.0f };
    m_context->OMSetRenderTargets(1, m_target.GetAddressOf(), nullptr);
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(desc.Width);
    viewport.Height = static_cast<float>(desc.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);
    m_context->ClearRenderTargetView(m_target.Get(), clear);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    ID3D11ShaderResourceView* view = m_guestView.Get();
    m_context->PSSetShaderResources(0, 1, &view);
    ID3D11SamplerState* sampler = m_sampler.Get();
    m_context->PSSetSamplers(0, 1, &sampler);
    m_context->Draw(3, 0);
    ID3D11ShaderResourceView* nullView = nullptr;
    m_context->PSSetShaderResources(0, 1, &nullView);

    hr = m_swapChain->Present(1, 0);
    if (FAILED(hr)) {
        error = HrText(L"Present da CoreWindow swap chain falhou", hr);
        return false;
    }
    return true;
}

}}
