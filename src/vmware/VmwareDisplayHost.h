#pragma once

#include "graphics/NativeRenderer.h"
#include "graphics/Win32D3D11Renderer.h"
#include "vmware/Win32Window.h"

#include <winrt/Windows.UI.Core.h>

namespace VMwareXbox
{
    // Coordinates the two platform window/render paths behind one API:
    // Xbox/UWP -> CoreWindow + CreateSwapChainForCoreWindow.
    // Desktop  -> HWND + CreateSwapChainForHwnd.
    // Only one backend is active in a process; they are not two simultaneous windows.
    class VmwareDisplayHost final
    {
    public:
        HRESULT InitializeCoreWindow(
            winrt::Windows::UI::Core::CoreWindow const& window);
        void RenderCoreWindowFrame();

        int RunDesktopWindow(
            const wchar_t* title = L"VMware-UWP-Xbox",
            int width = 1280,
            int height = 720);

        void Shutdown() noexcept;

    private:
        NativeRenderer m_coreRenderer;
        Win32D3D11Renderer m_win32Renderer;
        Win32WindowHost m_win32Window;
    };
}
