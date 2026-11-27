#include <windows.h>
#include <windows.applicationmodel.core.h>
#include <windows.ui.core.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;
using namespace Platform;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

namespace
{
    class OnyxFrameworkView sealed : public IFrameworkView
    {
    public:
        void Initialize(CoreApplicationView^)
        {
            OutputDebugStringW(L"[OnyxOS] CoreApplication initialized.\n");
        }

        void SetWindow(CoreWindow^ window)
        {
            m_window = window;
            m_window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(
                this, &OnyxFrameworkView::OnVisibilityChanged);
            m_window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(
                this, &OnyxFrameworkView::OnClosed);
            m_window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(
                this, &OnyxFrameworkView::OnKeyDown);
            m_window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
                this, &OnyxFrameworkView::OnPointerPressed);
            InitializeGraphics();
        }

        void Load(String^) {}

        void Run()
        {
            while (!m_closed)
            {
                if (m_visible)
                {
                    m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
                    Render();
                }
                else
                {
                    m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
                }
            }
        }

        void Uninitialize()
        {
            m_context.Reset();
            m_target.Reset();
            m_swapChain.Reset();
            m_device.Reset();
            OutputDebugStringW(L"[OnyxOS] CoreWindow shutdown complete.\n");
        }

    private:
        void InitializeGraphics()
        {
            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            const D3D_FEATURE_LEVEL levels[] = {
                D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
            };
            D3D_FEATURE_LEVEL selected{};
            HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                flags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                &m_device, &selected, &m_context);
            if (hr == E_INVALIDARG)
                hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                    flags, levels + 1, ARRAYSIZE(levels) - 1, D3D11_SDK_VERSION,
                    &m_device, &selected, &m_context);
            if (FAILED(hr))
            {
                LogHr(L"D3D11CreateDevice", hr);
                return;
            }

            ComPtr<IDXGIDevice> dxgiDevice;
            ComPtr<IDXGIAdapter> adapter;
            ComPtr<IDXGIFactory2> factory;
            if (FAILED(hr = m_device.As(&dxgiDevice)) ||
                FAILED(hr = dxgiDevice->GetAdapter(&adapter)) ||
                FAILED(hr = adapter->GetParent(IID_PPV_ARGS(&factory))))
            {
                LogHr(L"DXGI factory initialization", hr);
                return;
            }

            DXGI_SWAP_CHAIN_DESC1 desc{};
            desc.Width = 1280;
            desc.Height = 720;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 2;
            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            hr = factory->CreateSwapChainForCoreWindow(
                m_device.Get(), reinterpret_cast<IUnknown*>(m_window), &desc, nullptr, &m_swapChain);
            if (FAILED(hr))
            {
                LogHr(L"CreateSwapChainForCoreWindow", hr);
                return;
            }

            ComPtr<ID3D11Texture2D> backBuffer;
            if (FAILED(hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))) ||
                FAILED(hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_target)))
            {
                LogHr(L"Create render target", hr);
                return;
            }
            m_graphicsReady = true;
            OutputDebugStringW(L"[OnyxOS] CoreWindow Direct3D 11 renderer initialized.\n");
        }

        void Render()
        {
            if (!m_graphicsReady || !m_context || !m_target || !m_swapChain)
                return;
            const float background[] = { 0.035f, 0.055f, 0.085f, 1.0f };
            m_context->OMSetRenderTargets(1, m_target.GetAddressOf(), nullptr);
            m_context->ClearRenderTargetView(m_target.Get(), background);
            const HRESULT hr = m_swapChain->Present(1, 0);
            if (FAILED(hr) && hr != DXGI_ERROR_WAS_STILL_DRAWING)
                LogHr(L"IDXGISwapChain::Present", hr);
        }

        void OnVisibilityChanged(CoreWindow^, VisibilityChangedEventArgs^ args)
        {
            m_visible = args->Visible;
        }

        void OnClosed(CoreWindow^, CoreWindowEventArgs^)
        {
            m_closed = true;
        }

        void OnKeyDown(CoreWindow^, KeyEventArgs^ args)
        {
            wchar_t message[96]{};
            swprintf_s(message, L"[OnyxOS][Input] CoreWindow key: %u\n",
                static_cast<unsigned>(args->VirtualKey));
            OutputDebugStringW(message);
        }

        void OnPointerPressed(CoreWindow^, PointerEventArgs^ args)
        {
            const auto point = args->CurrentPoint->Position;
            wchar_t message[128]{};
            swprintf_s(message, L"[OnyxOS][Input] Pointer: %.1f, %.1f\n", point.X, point.Y);
            OutputDebugStringW(message);
        }

        static void LogHr(const wchar_t* operation, HRESULT hr)
        {
            wchar_t message[192]{};
            swprintf_s(message, L"[OnyxOS] %s failed: 0x%08X\n",
                operation, static_cast<unsigned>(hr));
            OutputDebugStringW(message);
        }

        CoreWindow^ m_window;
        ComPtr<ID3D11Device> m_device;
        ComPtr<ID3D11DeviceContext> m_context;
        ComPtr<IDXGISwapChain1> m_swapChain;
        ComPtr<ID3D11RenderTargetView> m_target;
        bool m_graphicsReady{ false };
        bool m_visible{ true };
        bool m_closed{ false };
    };

    ref class OnyxFrameworkViewSource sealed : IFrameworkViewSource
    {
    public:
        virtual IFrameworkView^ CreateView()
        {
            return ref new OnyxFrameworkView();
        }
    };
}

[MTAThread]
int main(Array<String^>^)
{
    CoreApplication::Run(ref new OnyxFrameworkViewSource());
    return 0;
}
