#include "VmwareDisplayHost.h"

#include <winapifamily.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <windows.h>
#endif

namespace VMwareXbox
{
    HRESULT VmwareDisplayHost::InitializeCoreWindow(
        winrt::Windows::UI::Core::CoreWindow const& window)
    {
        return m_coreRenderer.Initialize(window);
    }

    void VmwareDisplayHost::RenderCoreWindowFrame()
    {
        m_coreRenderer.Render();
    }

    int VmwareDisplayHost::RunDesktopWindow(
        const wchar_t* title, int width, int height)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (!m_win32Window.Create(title, width, height))
            return static_cast<int>(HRESULT_FROM_WIN32(GetLastError()));

        HRESULT hr = m_win32Renderer.Initialize(m_win32Window.Handle());
        if (FAILED(hr))
        {
            m_win32Window.Destroy();
            return static_cast<int>(hr);
        }

        m_win32Window.Show(SW_SHOW);
        MSG message{};
        bool running = true;

        while (running)
        {
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    running = false;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }

            if (running)
                m_win32Renderer.Render();
        }

        m_win32Renderer.Shutdown();
        m_win32Window.Destroy();
        return static_cast<int>(message.wParam);
#else
        (void)title;
        (void)width;
        (void)height;
        return static_cast<int>(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
#endif
    }

    void VmwareDisplayHost::Shutdown() noexcept
    {
        m_win32Renderer.Shutdown();
        m_win32Window.Destroy();
    }
}
