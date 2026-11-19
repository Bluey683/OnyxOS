#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Foundation.h>
#include "vm/QemuHostAdapter.h"
#include "vm/QemuInputBridge.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace winrt;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

struct OnyxOSView final : implements<OnyxOSView, IFrameworkView>
{
    void Initialize(CoreApplicationView const&) {}
    void SetWindow(CoreWindow const& window)
    {
        m_window=window;
        m_hostLoaded=m_host.Load();
        if(!m_hostLoaded)return;
        m_input=std::make_unique<onyx::vm::QemuInputBridge>(m_host);
        m_window.KeyDown([this](CoreWindow const&,KeyEventArgs const& e)
        { if(m_input)m_input->SendVirtualKey(static_cast<int>(e.VirtualKey()),true); });
        m_window.KeyUp([this](CoreWindow const&,KeyEventArgs const& e)
        { if(m_input)m_input->SendVirtualKey(static_cast<int>(e.VirtualKey()),false); });
        m_window.PointerMoved([this](CoreWindow const&,PointerEventArgs const& e)
        { if(m_input){auto p=e.CurrentPoint().Position();auto b=m_window.Bounds();m_input->SendPointerPosition(p.X,p.Y,b.Width,b.Height);} });
        m_window.PointerPressed([this](CoreWindow const&,PointerEventArgs const& e)
        {
            if(!m_input)return; auto p=e.CurrentPoint().Properties();
            if(p.IsLeftButtonPressed())m_input->SendPointerButton(0,true);
            if(p.IsMiddleButtonPressed())m_input->SendPointerButton(1,true);
            if(p.IsRightButtonPressed())m_input->SendPointerButton(2,true);
        });
        m_window.PointerReleased([this](CoreWindow const&,PointerEventArgs const&)
        { if(m_input){m_input->SendPointerButton(0,false);m_input->SendPointerButton(1,false);m_input->SendPointerButton(2,false);} });
    }
    void Load(hstring const&) {}
    void Run()
    {
        if(m_window)m_window.Activate();
        if(m_hostLoaded)
        {
            m_gamepadRunning=true;
            m_gamepadThread=std::thread([this]()
            {
                while(m_gamepadRunning)
                {
                    if(m_input)m_input->PollGamepad();
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }
            });
        }
        while(m_window&&m_window.Visible())
            m_window.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        m_gamepadRunning=false;
        if(m_gamepadThread.joinable())m_gamepadThread.join();
        if(m_input)m_input->ReleaseAll();
    }
    void Uninitialize()
    {
        m_gamepadRunning=false;
        if(m_gamepadThread.joinable())m_gamepadThread.join();
        if(m_input)m_input->ReleaseAll();
        m_input.reset(); m_host.Unload();
    }
private:
    CoreWindow m_window{nullptr};
    onyx::vm::QemuHostAdapter m_host;
    std::unique_ptr<onyx::vm::QemuInputBridge> m_input;
    bool m_hostLoaded{false};
    std::atomic_bool m_gamepadRunning{false};
    std::thread m_gamepadThread;
};

struct App final : implements<App,IFrameworkViewSource>
{ IFrameworkView CreateView(){return make<OnyxOSView>();} };

int __stdcall wWinMain(HINSTANCE,HINSTANCE,PWSTR,int)
{
    init_apartment(apartment_type::single_threaded);
    CoreApplication::Run(make<App>());
    return 0;
}
