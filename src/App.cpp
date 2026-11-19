#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Foundation.h>
#include <atomic>

using namespace winrt;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

struct OnyxOSView : implements<OnyxOSView, IFrameworkView>
{
    void Initialize(CoreApplicationView const&) {}

    void SetWindow(CoreWindow const& window)
    {
        m_window = window;
        // The launcher must not depend on the optional QEMU runtime being present.
        // All seven package entries share this host, so a missing guest DLL must
        // never terminate the app before its first window is shown.
        m_window.Closed([this](CoreWindow const&, CoreWindowEventArgs const&)
        {
            m_running = false;
        });
    }

    void Load(hstring const&) {}

    void Run()
    {
        if (!m_window) return;

        m_running = true;
        m_window.Activate();

        // Do not use CoreWindow::Visible as the process lifetime condition:
        // on some UWP activation paths it is initially false immediately after
        // the splash screen, which made the app exit straight away.
        while (m_running)
        {
            m_window.Dispatcher().ProcessEvents(
                CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }

    void Uninitialize()
    {
        m_running = false;
    }

private:
    CoreWindow m_window{ nullptr };
    std::atomic_bool m_running{ false };
};

struct App : implements<App, IFrameworkViewSource>
{
    IFrameworkView CreateView() { return make<OnyxOSView>(); }
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    try
    {
        init_apartment(apartment_type::single_threaded);
        CoreApplication::Run(make<App>());
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}
