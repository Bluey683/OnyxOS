#include "Win32Window.h"

#include <windows.h>
#include <winapifamily.h>

namespace VMwareXbox
{
    Win32WindowHost::~Win32WindowHost() noexcept
    {
        Destroy();
    }

    bool Win32WindowHost::Create(
        const wchar_t* title, int width, int height) noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (m_window || width <= 0 || height <= 0)
            return false;

        const wchar_t* className = L"VMwareUwpXbox.Win32Window";
        HINSTANCE instance = GetModuleHandleW(nullptr);

        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = &Win32WindowHost::WindowProcedure;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = className;

        if (!RegisterClassExW(&windowClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            return false;
        }

        m_window = CreateWindowExW(
            0,
            className,
            title ? title : L"VMware-UWP-Xbox",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            width,
            height,
            nullptr,
            nullptr,
            instance,
            this);

        return m_window != nullptr;
#else
        (void)title;
        (void)width;
        (void)height;
        // The Xbox UWP app partition does not expose desktop HWND creation.
        // Keep this explicit rather than silently pretending a second window exists.
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
#endif
    }

    void Win32WindowHost::Show(int showCommand) noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (m_window)
        {
            ShowWindow(m_window, showCommand);
            UpdateWindow(m_window);
        }
#else
        (void)showCommand;
#endif
    }

    void Win32WindowHost::Destroy() noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (m_window)
        {
            DestroyWindow(m_window);
            m_window = nullptr;
        }
#else
        m_window = nullptr;
#endif
    }

    LRESULT CALLBACK Win32WindowHost::WindowProcedure(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (message == WM_NCCREATE)
        {
            const auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(
                window,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }

        switch (message)
        {
        case WM_ERASEBKGND:
            return 1;
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
#else
        (void)window;
        (void)message;
        (void)wParam;
        (void)lParam;
        return 0;
#endif
    }
}
