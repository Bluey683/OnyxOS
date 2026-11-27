#include <windows.h>
#include <windows.applicationmodel.core.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <string>
#include "OnyxLogger.h"
#include "vm/VhdxProvisioner.h"

using namespace Platform;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

namespace OnyxOS
{
    public ref class OnyxFrameworkView sealed : IFrameworkView
    {
    public:
        virtual void Initialize(CoreApplicationView^ applicationView)
        {
            OutputDebugStringW(L"[OnyxOS] CoreApplication initialized.\n");
            OnyxLogger::Info(L"APP", L"CoreApplication initialized.");
            if (onyx::vm::EnsureDevelopmentFilesDirectories())
            {
                OnyxLogger::Info(L"STORAGE", L"Windows 11 LocalState folders are ready.");
                const auto disk = onyx::vm::FindDevelopmentFilesDisk("windows11");
                if (!disk.empty())
                    OnyxLogger::Info(L"STORAGE", L"QCOW2 template is available in LocalState; existing guest disks are preserved.");
                else
                    OnyxLogger::Error(L"STORAGE", L"QCOW2 template could not be copied or validated in LocalState.");
            }
            else
                OnyxLogger::Error(L"STORAGE", L"Could not create Windows 11 LocalState folders.");
        }

        virtual void SetWindow(CoreWindow^ window)
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

        virtual void Load(String^ entryPoint) { (void)entryPoint; }

        virtual void Run()
        {
            m_window->Activate();
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

        virtual void Uninitialize()
        {
            ReleaseGraphics();
            OutputDebugStringW(L"[OnyxOS] CoreWindow shutdown complete.\n");
            OnyxLogger::Info(L"APP", L"CoreWindow shutdown complete.");
        }

    private:
        void ReleaseGraphics()
        {
            if (m_target) { m_target->Release(); m_target = nullptr; }
            if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
            if (m_context) { m_context->Release(); m_context = nullptr; }
            if (m_device) { m_device->Release(); m_device = nullptr; }
            m_graphicsReady = false;
        }

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
            if (FAILED(hr)) { LogHr(L"D3D11CreateDevice", hr); return; }

            IDXGIDevice* dxgiDevice = nullptr;
            IDXGIAdapter* adapter = nullptr;
            IDXGIFactory2* factory = nullptr;
            hr = m_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
            if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr)) hr = adapter->GetParent(IID_PPV_ARGS(&factory));
            if (dxgiDevice) dxgiDevice->Release();
            if (adapter) adapter->Release();
            if (FAILED(hr)) { if (factory) factory->Release(); LogHr(L"DXGI factory initialization", hr); return; }

            DXGI_SWAP_CHAIN_DESC1 desc{};
            desc.Width = 1280; desc.Height = 720;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 2;
            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            hr = factory->CreateSwapChainForCoreWindow(
                m_device, reinterpret_cast<IUnknown*>(m_window), &desc, nullptr, &m_swapChain);
            factory->Release();
            if (FAILED(hr)) { LogHr(L"CreateSwapChainForCoreWindow", hr); return; }

            ID3D11Texture2D* backBuffer = nullptr;
            hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
            if (SUCCEEDED(hr))
                hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_target);
            if (backBuffer) backBuffer->Release();
            if (FAILED(hr)) { LogHr(L"Create render target", hr); ReleaseGraphics(); return; }

            m_graphicsReady = true;
            OutputDebugStringW(L"[OnyxOS] CoreWindow Direct3D 11 renderer initialized.\n");
            OnyxLogger::Info(L"GRAPHICS", L"CoreWindow Direct3D 11 renderer initialized.");
        }

        void Render()
        {
            if (!m_graphicsReady) return;
            const float background[] = { 0.035f, 0.055f, 0.085f, 1.0f };
            m_context->OMSetRenderTargets(1, &m_target, nullptr);
            m_context->ClearRenderTargetView(m_target, background);
            const HRESULT hr = m_swapChain->Present(1, 0);
            if (FAILED(hr) && hr != DXGI_ERROR_WAS_STILL_DRAWING)
                LogHr(L"IDXGISwapChain::Present", hr);
        }

        void OnVisibilityChanged(CoreWindow^, VisibilityChangedEventArgs^ args) { m_visible = args->Visible; }
        void OnClosed(CoreWindow^, CoreWindowEventArgs^) { m_closed = true; }

        void OnKeyDown(CoreWindow^, KeyEventArgs^ args)
        {
            wchar_t message[96]{};
            swprintf_s(message, L"[OnyxOS][Input] CoreWindow key: %u\n", static_cast<unsigned>(args->VirtualKey));
            OutputDebugStringW(message);
            OnyxLogger::Info(L"INPUT", ref new String(message));
        }

        void OnPointerPressed(CoreWindow^, PointerEventArgs^ args)
        {
            const auto point = args->CurrentPoint->Position;
            wchar_t message[128]{};
            swprintf_s(message, L"[OnyxOS][Input] Pointer: %.1f, %.1f\n", point.X, point.Y);
            OutputDebugStringW(message);
            OnyxLogger::Info(L"INPUT", ref new String(message));
        }

        static void LogHr(const wchar_t* operation, HRESULT hr)
        {
            wchar_t message[192]{};
            swprintf_s(message, L"[OnyxOS] %s failed: 0x%08X\n", operation, static_cast<unsigned>(hr));
            OutputDebugStringW(message);
            OnyxLogger::Error(L"GRAPHICS", ref new String(message));
        }

        CoreWindow^ m_window;
        ID3D11Device* m_device{ nullptr };
        ID3D11DeviceContext* m_context{ nullptr };
        IDXGISwapChain1* m_swapChain{ nullptr };
        ID3D11RenderTargetView* m_target{ nullptr };
        bool m_graphicsReady{ false };
        bool m_visible{ true };
        bool m_closed{ false };
    };

    public ref class OnyxFrameworkViewSource sealed : IFrameworkViewSource
    {
    public:
        virtual IFrameworkView^ CreateView() { return ref new OnyxFrameworkView(); }
    };
}

[MTAThread]
int main(Array<String^>^)
{
    CoreApplication::Run(ref new OnyxOS::OnyxFrameworkViewSource());
    return 0;
}
