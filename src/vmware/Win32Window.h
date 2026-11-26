#pragma once

#include <windows.h>

namespace VMwareXbox
{
    // Native Win32 top-level window host.
    // Desktop builds create an HWND. Xbox/UWP builds compile the same interface,
    // but report that desktop top-level window creation is unavailable.
    class Win32WindowHost final
    {
    public:
        Win32WindowHost() noexcept = default;
        ~Win32WindowHost() noexcept;

        Win32WindowHost(const Win32WindowHost&) = delete;
        Win32WindowHost& operator=(const Win32WindowHost&) = delete;

        bool Create(const wchar_t* title, int width, int height) noexcept;
        void Show(int showCommand = SW_SHOW) noexcept;
        void Destroy() noexcept;

        HWND Handle() const noexcept { return m_window; }
        bool IsCreated() const noexcept { return m_window != nullptr; }

    private:
        static LRESULT CALLBACK WindowProcedure(
            HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

        HWND m_window = nullptr;
    };
}
