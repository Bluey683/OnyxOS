#include "App.h"
#include "vmware/VmwareDisplayHost.h"

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>

namespace
{
    struct NativeAppView final :
        winrt::implements<NativeAppView, winrt::Windows::ApplicationModel::Core::IFrameworkView>
    {
        void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const&) {}

        void SetWindow(winrt::Windows::UI::Core::CoreWindow const& window)
        {
            m_window = window;
            m_window.Closed([this](
                winrt::Windows::UI::Core::CoreWindow const&,
                winrt::Windows::UI::Core::CoreWindowEventArgs const&)
            {
                m_closed = true;
            });
        }

        void Load(winrt::hstring const&) {}

        void Run()
        {
            m_window.Activate();

            const HRESULT initResult = m_display.InitializeCoreWindow(m_window);
            if (FAILED(initResult))
                return;

            while (!m_closed)
            {
                m_window.Dispatcher().ProcessEvents(
                    winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessAll);
                if (!m_closed)
                    m_display.RenderCoreWindowFrame();
            }
        }

        void Uninitialize()
        {
            m_display.Shutdown();
            m_window = nullptr;
        }

    private:
        winrt::Windows::UI::Core::CoreWindow m_window{ nullptr };
        VMwareXbox::VmwareDisplayHost m_display;
        bool m_closed{ false };
    };
}

namespace winrt::OnyxOS::implementation
{
    Windows::ApplicationModel::Core::IFrameworkView App::CreateView()
    {
        return winrt::make<NativeAppView>();
    }
}
