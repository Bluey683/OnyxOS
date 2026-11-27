#include <windows.h>
#include <windows.applicationmodel.core.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <string>
#include "OnyxLogger.h"
#include "vm/VhdxProvisioner.h"
#include "vm/VirtualMachine.h"
#include "vm/QemuHostAdapter.h"
#include "vm/QemuInputBridge.h"
#include <roapi.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <cstdint>

using namespace Platform;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

namespace OnyxOS
{
    struct GuestFrameState
    {
        std::mutex mutex;
        std::vector<std::uint32_t> pixels;
        int width{ 0 };
        int height{ 0 };
        std::atomic<bool> firstFrameLogged{ false };
    };

    static std::string ToUtf8(const std::wstring& value)
    {
        if (value.empty()) return {};
        const int length = WideCharToMultiByte(CP_UTF8, 0, value.c_str(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (length <= 0) return {};
        std::string result(static_cast<std::size_t>(length), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()),
            result.data(), length, nullptr, nullptr);
        return result;
    }

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
            m_window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(
                this, &OnyxFrameworkView::OnKeyUp);
            m_window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
                this, &OnyxFrameworkView::OnPointerMoved);
            m_window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
                this, &OnyxFrameworkView::OnPointerPressed);
            m_window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
                this, &OnyxFrameworkView::OnPointerReleased);
            InitializeGraphics();
            StartVirtualMachine();
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
                    if (m_input && m_qemu && m_qemu->IsRunning()) m_input->PollGamepad();
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
            if (m_qemu) m_qemu->Stop();
            if (m_vmStartThread && m_vmStartThread->joinable()) m_vmStartThread->join();
            delete m_vmStartThread; m_vmStartThread = nullptr;
            delete m_input; m_input = nullptr;
            delete m_qemu; m_qemu = nullptr;
            delete m_frameState; m_frameState = nullptr;
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

        void StartVirtualMachine()
        {
            const auto disk = onyx::vm::FindDevelopmentFilesDisk("windows11");
            if (disk.empty())
            {
                OnyxLogger::Error(L"VM", L"VM start cancelled: Windows 11 QCOW2 disk is unavailable.");
                return;
            }

            const auto iso = onyx::vm::FindDevelopmentFilesIso("windows11");
            auto configuration = onyx::vm::CreateProfile("windows11");
            configuration.diskPath = ToUtf8(disk);
            configuration.isoPath = ToUtf8(iso);
            configuration.useUefi = true;
            configuration.tcgJitCacheMB = 128;

            const auto validation = onyx::vm::Validate(configuration);
            if (!validation.valid)
            {
                const std::wstring message(validation.message.begin(), validation.message.end());
                OnyxLogger::Error(L"VM", ref new String(message.c_str()));
                return;
            }

            const auto arguments = onyx::vm::BuildQemuArguments(configuration);
            if (arguments.empty())
            {
                OnyxLogger::Error(L"VM", L"VM start cancelled: QEMU argument generation returned an empty list.");
                return;
            }

            OnyxLogger::Info(L"VM", L"Starting Windows 11 x64 VM: 3072 MB RAM, 1 vCPU, TCG JIT cache 128 MB.");
            OnyxLogger::Info(L"VM", L"QEMU TCG JIT acceleration requested (single-threaded).");
            OnyxLogger::Info(L"VM", iso.empty()
                ? L"No ISO found in LocalState\\Windows11\\ISO; attempting boot from the existing QCOW2 disk."
                : L"Windows installer ISO detected and will be attached as the first boot device.");
            OnyxLogger::Info(L"VM", L"UEFI firmware: qemu/edk2-x86_64-code.fd.");

            m_qemu = new onyx::vm::QemuHostAdapter();
            m_input = new onyx::vm::QemuInputBridge(*m_qemu);
            m_frameState = new GuestFrameState();
            auto* host = m_qemu;
            auto* frameState = m_frameState;
            m_vmStartThread = new std::thread([host, frameState, arguments]()
            {
                const HRESULT apartment = RoInitialize(RO_INIT_MULTITHREADED);
                if (FAILED(apartment))
                {
                    OnyxOS::WriteQemuLogNative(L"[BOOT] RoInitialize(MTA) failed on VM startup thread.");
                    return;
                }

                OnyxOS::WriteQemuLogNative(L"[BOOT] VM startup requested.");
                OnyxOS::WriteQemuLogNative(L"[BOOT] Loading packaged QEMU-UWP-Host DLL.");
                if (!host->Load())
                {
                    OnyxOS::WriteQemuLogNative((L"[ERROR] QEMU DLL load failed: " + host->Error()).c_str());
                    RoUninitialize();
                    return;
                }

                OnyxOS::WriteQemuLogNative(L"[BOOT] QEMU DLL loaded; validating exported host API.");
                OnyxOS::WriteQemuLogNative(L"[BOOT] Attaching QCOW2 disk and optional installer ISO.");
                OnyxOS::WriteQemuLogNative(L"[BOOT] Starting QEMU TCG JIT engine.");
                const bool started = host->Start(arguments,
                    [frameState](const std::vector<std::uint32_t>& pixels, int width, int height)
                    {
                        {
                            std::lock_guard<std::mutex> lock(frameState->mutex);
                            frameState->pixels = pixels;
                            frameState->width = width;
                            frameState->height = height;
                        }
                        if (!frameState->firstFrameLogged.exchange(true))
                            OnyxOS::WriteQemuLogNative(L"[VIDEO] First guest framebuffer received from QEMU.");
                    });

                if (started)
                    OnyxOS::WriteQemuLogNative(L"[BOOT] VM is running; first guest frame received.");
                else
                    OnyxOS::WriteQemuLogNative((L"[ERROR] VM startup failed: " + host->Error()).c_str());
                RoUninitialize();
            });
        }

        void Render()
        {
            if (!m_graphicsReady) return;
            const float background[] = { 0.035f, 0.055f, 0.085f, 1.0f };
            m_context->OMSetRenderTargets(1, &m_target, nullptr);

            bool drewGuest = false;
            if (m_frameState)
            {
                std::vector<std::uint32_t> source;
                int sourceWidth = 0, sourceHeight = 0;
                {
                    std::lock_guard<std::mutex> lock(m_frameState->mutex);
                    source = m_frameState->pixels;
                    sourceWidth = m_frameState->width;
                    sourceHeight = m_frameState->height;
                }
                if (!source.empty() && sourceWidth > 0 && sourceHeight > 0)
                {
                    ID3D11Texture2D* backBuffer = nullptr;
                    if (SUCCEEDED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
                    {
                        D3D11_TEXTURE2D_DESC desc{};
                        backBuffer->GetDesc(&desc);
                        std::vector<std::uint32_t> scaled(static_cast<std::size_t>(desc.Width) * desc.Height);
                        for (UINT y = 0; y < desc.Height; ++y)
                        {
                            const int sy = std::min(sourceHeight - 1,
                                static_cast<int>((static_cast<std::uint64_t>(y) * sourceHeight) / desc.Height));
                            for (UINT x = 0; x < desc.Width; ++x)
                            {
                                const int sx = std::min(sourceWidth - 1,
                                    static_cast<int>((static_cast<std::uint64_t>(x) * sourceWidth) / desc.Width));
                                scaled[static_cast<std::size_t>(y) * desc.Width + x] =
                                    source[static_cast<std::size_t>(sy) * sourceWidth + sx];
                            }
                        }
                        m_context->UpdateSubresource(backBuffer, 0, nullptr, scaled.data(),
                            desc.Width * sizeof(std::uint32_t), 0);
                        backBuffer->Release();
                        drewGuest = true;
                    }
                }
            }
            if (!drewGuest) m_context->ClearRenderTargetView(m_target, background);
            const HRESULT hr = m_swapChain->Present(1, 0);
            if (FAILED(hr) && hr != DXGI_ERROR_WAS_STILL_DRAWING)
                LogHr(L"IDXGISwapChain::Present", hr);
        }

        void OnVisibilityChanged(CoreWindow^, VisibilityChangedEventArgs^ args) { m_visible = args->Visible; }
        void OnClosed(CoreWindow^, CoreWindowEventArgs^) { m_closed = true; }

        void OnKeyDown(CoreWindow^, KeyEventArgs^ args)
        {
            if (m_input) m_input->SendVirtualKey(static_cast<int>(args->VirtualKey), true);
            wchar_t message[96]{};
            swprintf_s(message, L"[OnyxOS][Input] Key down: %u\n", static_cast<unsigned>(args->VirtualKey));
            OutputDebugStringW(message);
            OnyxLogger::Info(L"INPUT", ref new String(message));
        }

        void OnKeyUp(CoreWindow^, KeyEventArgs^ args)
        {
            if (m_input) m_input->SendVirtualKey(static_cast<int>(args->VirtualKey), false);
        }

        void OnPointerMoved(CoreWindow^, PointerEventArgs^ args)
        {
            if (!m_input) return;
            const auto point = args->CurrentPoint->Position;
            m_input->SendPointerPosition(point.X, point.Y, 1280.0f, 720.0f);
        }

        void OnPointerPressed(CoreWindow^, PointerEventArgs^ args)
        {
            if (!m_input) return;
            const auto point = args->CurrentPoint->Position;
            m_input->SendPointerPosition(point.X, point.Y, 1280.0f, 720.0f);
            const auto properties = args->CurrentPoint->Properties;
            if (properties->IsLeftButtonPressed) m_input->SendPointerButton(0, true);
            else if (properties->IsRightButtonPressed) m_input->SendPointerButton(2, true);
            else m_input->SendPointerButton(0, true);
        }

        void OnPointerReleased(CoreWindow^, PointerEventArgs^)
        {
            if (m_input)
            {
                m_input->SendPointerButton(0, false);
                m_input->SendPointerButton(2, false);
            }
        }

        static void LogHr(const wchar_t* operation, HRESULT hr)
        {
            wchar_t message[192]{};
            swprintf_s(message, L"[OnyxOS] %s failed: 0x%08X\n", operation, static_cast<unsigned>(hr));
            OutputDebugStringW(message);
            OnyxLogger::Error(L"GRAPHICS", ref new String(message));
        }

        CoreWindow^ m_window;
        onyx::vm::QemuHostAdapter* m_qemu{ nullptr };
        onyx::vm::QemuInputBridge* m_input{ nullptr };
        GuestFrameState* m_frameState{ nullptr };
        std::thread* m_vmStartThread{ nullptr };
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
