#include "QemuD3DRenderer.h"
#include <d3dcompiler.h>
#include <windows.ui.xaml.media.dxinterop.h>
#include <cstring>
#include <sstream>

using Microsoft::WRL::ComPtr;

namespace onyx { namespace graphics {
namespace {
const char* ShaderSource = R"(
Texture2D GuestFramebuffer : register(t0);
SamplerState GuestSampler : register(s0);
struct VOut { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
VOut VSMain(uint id : SV_VertexID) {
    VOut o;
    float2 p = float2((id == 2) ? 3.0 : -1.0, (id == 1) ? 3.0 : -1.0);
    o.position = float4(p, 0.0, 1.0);
    o.uv = float2((p.x + 1.0) * 0.5, (1.0 - p.y) * 0.5);
    return o;
}
float4 PSMain(VOut input) : SV_TARGET { return GuestFramebuffer.Sample(GuestSampler, input.uv); }
)";
std::wstring HrText(const wchar_t* operation, HRESULT hr) {
    std::wstringstream s; s << operation << L" (HRESULT 0x" << std::hex << static_cast<unsigned long>(hr) << L")"; return s.str();
}
}
bool QemuD3DRenderer::Initialize(Windows::UI::Xaml::Controls::SwapChainPanel^ panel, std::wstring& error)
{
    if (!panel) { error = L"SwapChainPanel ausente."; return false; }
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL selected{};
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, ARRAYSIZE(levels),
        D3D11_SDK_VERSION, &m_device, &selected, &m_context);
    if (hr == E_INVALIDARG) hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels + 1, ARRAYSIZE(levels) - 1,
        D3D11_SDK_VERSION, &m_device, &selected, &m_context);
    if (FAILED(hr)) { error = HrText(L"D3D11CreateDevice falhou", hr); return false; }
    ComPtr<IDXGIDevice> dxgiDevice; ComPtr<IDXGIAdapter> adapter; ComPtr<IDXGIFactory2> factory;
    if (FAILED(hr = m_device.As(&dxgiDevice)) || FAILED(hr = dxgiDevice->GetAdapter(&adapter)) ||
        FAILED(hr = adapter->GetParent(IID_PPV_ARGS(&factory)))) { error = HrText(L"Obtenção da fábrica DXGI falhou", hr); return false; }
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width=1920; desc.Height=1080; desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count=1; desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount=2;
    desc.Scaling=DXGI_SCALING_STRETCH; desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; desc.AlphaMode=DXGI_ALPHA_MODE_IGNORE;
    if (FAILED(hr=factory->CreateSwapChainForComposition(m_device.Get(),&desc,nullptr,&m_swapChain))) { error=HrText(L"CreateSwapChainForComposition falhou",hr); return false; }
    ComPtr<ISwapChainPanelNative> nativePanel;
    if (FAILED(hr=reinterpret_cast<IUnknown*>(panel)->QueryInterface(IID_PPV_ARGS(&nativePanel))) ||
        FAILED(hr=nativePanel->SetSwapChain(m_swapChain.Get()))) { error=HrText(L"SwapChainPanel::SetSwapChain falhou",hr); return false; }
    ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(hr=m_swapChain->GetBuffer(0,IID_PPV_ARGS(&backBuffer))) ||
        FAILED(hr=m_device->CreateRenderTargetView(backBuffer.Get(),nullptr,&m_target))) { error=HrText(L"Render target falhou",hr); return false; }
    ComPtr<ID3DBlob> vs,ps,errors;
    hr=D3DCompile(ShaderSource,strlen(ShaderSource),nullptr,nullptr,nullptr,"VSMain","vs_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&vs,&errors);
    if (FAILED(hr)) { error=L"Compilação do vertex shader HLSL falhou."; return false; }
    hr=D3DCompile(ShaderSource,strlen(ShaderSource),nullptr,nullptr,nullptr,"PSMain","ps_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&ps,&errors);
    if (FAILED(hr)) { error=L"Compilação do pixel shader HLSL falhou."; return false; }
    if (FAILED(hr=m_device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&m_vertexShader)) ||
        FAILED(hr=m_device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&m_pixelShader))) { error=HrText(L"Criação dos shaders falhou",hr); return false; }
    D3D11_SAMPLER_DESC sd{}; sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP; sd.MaxLOD=D3D11_FLOAT32_MAX;
    if (FAILED(hr=m_device->CreateSamplerState(&sd,&m_sampler))) { error=HrText(L"Criação do sampler falhou",hr); return false; }
    return true;
}
bool QemuD3DRenderer::CreateGuestTexture(int width,int height,std::wstring& error)
{
    m_guestView.Reset(); m_guestTexture.Reset(); D3D11_TEXTURE2D_DESC td{};
    td.Width=static_cast<UINT>(width); td.Height=static_cast<UINT>(height); td.MipLevels=1; td.ArraySize=1;
    td.Format=DXGI_FORMAT_B8G8R8A8_UNORM; td.SampleDesc.Count=1; td.Usage=D3D11_USAGE_DEFAULT; td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr=m_device->CreateTexture2D(&td,nullptr,&m_guestTexture);
    if(FAILED(hr)){error=HrText(L"Textura do framebuffer falhou",hr);return false;}
    hr=m_device->CreateShaderResourceView(m_guestTexture.Get(),nullptr,&m_guestView);
    if(FAILED(hr)){error=HrText(L"ShaderResourceView falhou",hr);return false;}
    m_guestWidth=width;m_guestHeight=height;return true;
}
bool QemuD3DRenderer::Present(const std::vector<std::uint32_t>& bgra,int width,int height,std::wstring& error)
{
    if(!m_device||width<=0||height<=0||bgra.size()!=static_cast<size_t>(width)*height){error=L"Framebuffer BGRA inválido.";return false;}
    if(width!=m_guestWidth||height!=m_guestHeight)if(!CreateGuestTexture(width,height,error))return false;
    m_context->UpdateSubresource(m_guestTexture.Get(),0,nullptr,bgra.data(),static_cast<UINT>(width*4),0);
    const float clear[4]={0.02f,0.02f,0.03f,1.0f};m_context->OMSetRenderTargets(1,m_target.GetAddressOf(),nullptr);
    // Fit the guest framebuffer inside the actual swap-chain bounds.
    // Never stretch/crop a 4:3 or unusual guest mode across the 16:9 Xbox panel.
    DXGI_SWAP_CHAIN_DESC1 swapDesc{};
    m_swapChain->GetDesc1(&swapDesc);
    const float targetWidth = static_cast<float>(swapDesc.Width);
    const float targetHeight = static_cast<float>(swapDesc.Height);
    const float guestAspect = static_cast<float>(width) / static_cast<float>(height);
    const float targetAspect = targetWidth / targetHeight;
    D3D11_VIEWPORT vp{};
    if (guestAspect > targetAspect)
    {
        vp.Width = targetWidth;
        vp.Height = targetWidth / guestAspect;
        vp.TopLeftY = (targetHeight - vp.Height) * 0.5f;
    }
    else
    {
        vp.Height = targetHeight;
        vp.Width = targetHeight * guestAspect;
        vp.TopLeftX = (targetWidth - vp.Width) * 0.5f;
    }
    vp.MinDepth=0.0f;vp.MaxDepth=1.0f;
    m_context->RSSetViewports(1,&vp);m_context->ClearRenderTargetView(m_target.Get(),clear);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vertexShader.Get(),nullptr,0);m_context->PSSetShader(m_pixelShader.Get(),nullptr,0);
    ID3D11ShaderResourceView* view=m_guestView.Get();m_context->PSSetShaderResources(0,1,&view);
    ID3D11SamplerState* sampler=m_sampler.Get();m_context->PSSetSamplers(0,1,&sampler);m_context->Draw(3,0);
    ID3D11ShaderResourceView* nullView=nullptr;m_context->PSSetShaderResources(0,1,&nullView);
    HRESULT hr=m_swapChain->Present(1,0);if(FAILED(hr)){error=HrText(L"Present falhou",hr);return false;}return true;
}
}}
