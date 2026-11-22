#include "graphics/CoreWindowD3DRenderer.h"
#include "Windows7AutoBoot.h"
#include <windows.h>
#include <windows.applicationmodel.core.h>
#include <collection.h>
#include <memory>
#include <mutex>
#include <vector>

using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
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
                OutputDebugStringW((L"[OnyxOS] CoreWindow renderer init failed: " + error + L"\n").c_str());
                m_renderer.reset();
            }

            // First launch downloads Windows 7 into LocalState/Disks. Later
            // launches reuse the same QCOW2 and boot it without a menu.
            m_autoBoot = std::make_unique<onyx::app::Windows7AutoBoot>(
                [this](const std::vector<std::uint32_t>& frame, int frameWidth, int frameHeight)
                {
                    std::lock_guard<std::mutex> guard(m_frameMutex);
                    m_latestFrame = frame;
                    m_frameWidth = frameWidth;
                    m_frameHeight = frameHeight;
                    m_frameDirty = true;
                });
            m_autoBoot->Start();
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
                        OutputDebugStringW((L"[OnyxOS] Guest framebuffer present failed: " + error + L"\n").c_str());
                        m_renderer.reset();
                    }
                }
                Sleep(1);
            }
        }

        virtual void Uninitialize()
        {
            m_running = false;
            if (m_autoBoot) m_autoBoot->Stop();
            m_autoBoot.reset();
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
                OutputDebugStringW((L"[OnyxOS] Resize failed: " + error + L"\n").c_str());
        }

        CoreWindow^ m_window;
        std::shared_ptr<onyx::graphics::CoreWindowD3DRenderer> m_renderer;
        std::unique_ptr<onyx::app::Windows7AutoBoot> m_autoBoot;
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
