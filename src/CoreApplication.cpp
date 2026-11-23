#include "graphics/CoreWindowD3DRenderer.h"
#include "vm/QemuHostAdapter.h"
#include "vm/VirtualMachine.h"
#include "vm/Windows7Boot.h"
#include "OnyxNativeLog.h"
#include <windows.h>
#include <windows.applicationmodel.core.h>
#include <windows.storage.h>
#include <collection.h>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Platform;

namespace OnyxOS
{
    public ref class App sealed : IFrameworkView
    {
    public:
        virtual void Initialize(CoreApplicationView^ applicationView)
        {
        }

        virtual void SetWindow(CoreWindow^ window)
        {
            m_window = window;
            m_window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(
                this, &App::OnVisibilityChanged);
            m_window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
                this, &App::OnSizeChanged);

            auto bounds = m_window->Bounds;
            const auto width = static_cast<unsigned>(bounds.Width > 0 ? bounds.Width : 1280);
            const auto height = static_cast<unsigned>(bounds.Height > 0 ? bounds.Height : 720);
            std::wstring error;
            m_renderer = std::make_shared<onyx::graphics::CoreWindowD3DRenderer>();
            if (!m_renderer->Initialize(m_window, width, height, error))
            {
                WriteQemuLogNative((L"[OnyxOS] CoreWindow renderer init failed: " + error).c_str());
                m_renderer.reset();
                return;
            }

            // Start guest preparation on a worker so the CoreWindow keeps
            // processing events while the multi-gigabyte QCOW2 is downloaded.
            m_qemu = std::make_unique<onyx::vm::QemuHostAdapter>();
            const std::wstring localStatePath(
                ApplicationData::Current->LocalFolder->Path->Data());

            m_vmStartupThread = std::thread([this, localStatePath]()
            {
                std::wstring diskPath;
                std::wstring bootError;
                if (!onyx::vm::PrepareWindows7Disk(localStatePath, diskPath, bootError))
                {
                    WriteQemuLogNative((L"[OnyxOS] Windows 7 disk setup failed: " + bootError).c_str());
                    return;
                }

                if (!m_qemu->Load())
                {
                    WriteQemuLogNative((L"[OnyxOS] QEMU host DLL load failed: " + m_qemu->Error()).c_str());
                    return;
                }

                onyx::vm::VmConfiguration config = onyx::vm::CreateProfile("windows7");
                config.diskPath.assign(diskPath.begin(), diskPath.end());
                const auto validation = onyx::vm::Validate(config);
                if (!validation.valid)
                {
                    WriteQemuLogNative((L"[OnyxOS] Windows 7 VM profile invalid: " +
                        std::wstring(validation.message.begin(), validation.message.end())).c_str());
                    return;
                }

                const auto arguments = onyx::vm::BuildQemuArguments(config);
                if (arguments.empty())
                {
                    WriteQemuLogNative(L"[OnyxOS] QEMU argument builder returned no arguments.");
                    return;
                }

                WriteQemuLogNative(L"[OnyxOS] Starting Windows 7 x64 guest through QEMU UWP host.");
                const bool started = m_qemu->Start(arguments,
                    [this](const std::vector<std::uint32_t>& frame, int frameWidth, int frameHeight)
                    {
                        std::lock_guard<std::mutex> guard(m_frameMutex);
                        m_latestFrame = frame;
                        m_frameWidth = frameWidth;
                        m_frameHeight = frameHeight;
                        m_frameDirty = true;
                    });

                if (!started)
                    WriteQemuLogNative((L"[OnyxOS] Windows 7 QEMU start failed: " + m_qemu->Error()).c_str());
                else
                    WriteQemuLogNative(L"[OnyxOS] First Windows 7 framebuffer received; guest display is active.");
            });
        }

        virtual void Load(String^ entryPoint)
        {
        }

        virtual void Run()
        {
            if (!m_window) return;
            m_window->Activate();
            std::vector<std::uint32_t> displayFrame;
            int displayWidth = 0;
            int displayHeight = 0;

            while (m_running)
            {
                m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
                {
                    std::lock_guard<std::mutex> guard(m_frameMutex);
                    if (m_frameDirty)
                    {
                        displayFrame.swap(m_latestFrame);
                        displayWidth = m_frameWidth;
                        displayHeight = m_frameHeight;
                        m_frameDirty = false;
                    }
                }
                if (m_renderer && !displayFrame.empty())
                {
                    std::wstring error;
                    if (!m_renderer->Present(displayFrame, displayWidth, displayHeight, error))
                    {
                        WriteQemuLogNative((L"[OnyxOS] Guest framebuffer present failed: " + error).c_str());
                        m_renderer.reset();
                    }
                    displayFrame.clear();
                }
                Sleep(1);
            }
        }

        virtual void Uninitialize()
        {
            m_running = false;
            // The startup worker may be downloading the guest disk or waiting
            // for QEMU's first frame. Join it before destroying the host adapter.
            if (m_vmStartupThread.joinable())
                m_vmStartupThread.join();
            if (m_qemu)
                m_qemu->Stop();
            m_qemu.reset();
            m_renderer.reset();
            m_window = nullptr;
        }

    private:
        void OnVisibilityChanged(CoreWindow^, VisibilityChangedEventArgs^ args)
        {
            m_visible = args->Visible;
        }

        void OnSizeChanged(CoreWindow^, WindowSizeChangedEventArgs^ args)
        {
            if (!m_renderer) return;
            const auto width = static_cast<unsigned>(args->Size.Width);
            const auto height = static_cast<unsigned>(args->Size.Height);
            std::wstring error;
            if (width >= 64 && height >= 64 && !m_renderer->Resize(width, height, error))
                WriteQemuLogNative((L"[OnyxOS] Resize failed: " + error).c_str());
        }

        CoreWindow^ m_window;
        std::shared_ptr<onyx::graphics::CoreWindowD3DRenderer> m_renderer;
        std::unique_ptr<onyx::vm::QemuHostAdapter> m_qemu;
        std::thread m_vmStartupThread;
        std::mutex m_frameMutex;
        std::vector<std::uint32_t> m_latestFrame;
        int m_frameWidth{ 0 };
        int m_frameHeight{ 0 };
        bool m_frameDirty{ false };
        bool m_running{ true };
        bool m_visible{ true };
    };

    public ref class AppSource sealed : IFrameworkViewSource
    {
    public:
        virtual IFrameworkView^ CreateView()
        {
            return ref new App();
        }
    };
}

[MTAThread]
int main(Array<String^>^)
{
    CoreApplication::Run(ref new OnyxOS::AppSource());
    return 0;
}
