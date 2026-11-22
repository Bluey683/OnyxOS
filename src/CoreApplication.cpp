#include "graphics/CoreWindowD3DRenderer.h"
#include <windows.h>
#include <windows.applicationmodel.core.h>
#include <collection.h>
#include <memory>
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
        }

        virtual void Load(String^ entryPoint)
        {
        }

        virtual void Run()
        {
            if (!m_window) return;
            m_window->Activate();

            // Until QEMU supplies its first BGRA frame, display a neutral
            // framebuffer through the same full-window D3D11/HLSL path.
            const std::vector<std::uint32_t> blank(4, 0xFF080D16u);
            while (m_running)
            {
                m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
                if (m_renderer)
                {
                    std::wstring error;
                    if (!m_renderer->Present(blank, 2, 2, error))
                    {
                        OutputDebugStringW((L"[OnyxOS] Present failed: " + error + L"\n").c_str());
                        m_renderer.reset();
                    }
                }
                Sleep(1);
            }
        }

        virtual void Uninitialize()
        {
            m_running = false;
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
