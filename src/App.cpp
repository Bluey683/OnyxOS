#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>

using namespace winrt;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

struct OnyxOSView final : implements<OnyxOSView, IFrameworkView>
{
    void Initialize(CoreApplicationView const&) {}
    void SetWindow(CoreWindow const& window) { m_window = window; }
    void Load(hstring const&) {}
    void Run()
    {
        if (m_window) m_window.Activate();
        while (m_window && m_window.Visible())
            m_window.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
    }
    void Uninitialize() {}
private:
    CoreWindow m_window{ nullptr };
};

struct App final : implements<App, IFrameworkViewSource>
{
    IFrameworkView CreateView() { return make<OnyxOSView>(); }
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    init_apartment(apartment_type::single_threaded);
    CoreApplication::Run(make<App>());
    return 0;
}
