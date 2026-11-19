#include "QemuInputBridge.h"
#include <cmath>
#include <algorithm>
#include <winrt/Windows.Gaming.Input.h>

using namespace winrt;
using namespace Windows::Gaming::Input;

namespace onyx::vm
{
    int QemuInputBridge::MapVirtualKey(int key) noexcept
    {
        if (key >= 0x30 && key <= 0x39) return key;
        if (key >= 0x41 && key <= 0x5A) return key + 0x20;
        if (key >= 0x70 && key <= 0x7B) return 282 + key - 0x70;
        switch (key)
        {
        case 0x08: return 8; case 0x09: return 9; case 0x0D: return 13;
        case 0x10: return 304; case 0x11: return 306; case 0x12: return 308;
        case 0x1B: return 27; case 0x20: return 32; case 0x21: return 280;
        case 0x22: return 281; case 0x23: return 279; case 0x24: return 278;
        case 0x25: return 276; case 0x26: return 273; case 0x27: return 275;
        case 0x28: return 274; case 0x2D: return 277; case 0x2E: return 127;
        case 0x14: return 301; default: return 0;
        }
    }

    void QemuInputBridge::SendVirtualKey(int key, bool down) noexcept
    {
        const int mapped = MapVirtualKey(key);
        if (mapped) { m_host.SendKeyNumber(mapped, down); m_host.WakeMainLoop(); }
    }

    void QemuInputBridge::SendPointerPosition(float x, float y, float width, float height) noexcept
    {
        m_surfaceWidth=(width>1?width:1920); m_surfaceHeight=(height>1?height:1080);
        m_gamepadX=x; m_gamepadY=y;
        if (!m_havePointer) { m_lastX=x; m_lastY=y; m_havePointer=true; return; }
        const int dx=static_cast<int>(std::lround(x-m_lastX));
        const int dy=static_cast<int>(std::lround(y-m_lastY));
        m_lastX=x; m_lastY=y;
        if (dx || dy)
        {
            if (!m_host.SendPointerRelative(dx,dy))
                m_host.SendPointerAbsolute(static_cast<int>(x),static_cast<int>(y),static_cast<int>(m_surfaceWidth),static_cast<int>(m_surfaceHeight));
            m_host.WakeMainLoop();
        }
    }

    void QemuInputBridge::SendPointerButton(int button, bool down) noexcept
    {
        if (button>=1 && button<=3) { m_host.SendPointerButton(button,down); m_host.WakeMainLoop(); }
    }

    void QemuInputBridge::UpdateGamepadKey(std::uint32_t mask, int key, std::uint32_t current) noexcept
    {
        const bool was=(m_previousButtons&mask)!=0, now=(current&mask)!=0;
        if (was!=now) m_host.SendKeyNumber(key,now);
    }

    void QemuInputBridge::PollGamepad() noexcept
    {
        const auto pads=Gamepad::Gamepads();
        if (!pads || pads.Size()==0) { ReleaseAll(); return; }
        const auto reading=pads.GetAt(0).GetCurrentReading();
        const auto bits=static_cast<std::uint32_t>(reading.Buttons);
        const auto mask=[](GamepadButtons b){return static_cast<std::uint32_t>(b);};
        UpdateGamepadKey(mask(GamepadButtons::DPadUp),273,bits);
        UpdateGamepadKey(mask(GamepadButtons::DPadDown),274,bits);
        UpdateGamepadKey(mask(GamepadButtons::DPadLeft),276,bits);
        UpdateGamepadKey(mask(GamepadButtons::DPadRight),275,bits);
        UpdateGamepadKey(mask(GamepadButtons::A),13,bits);
        UpdateGamepadKey(mask(GamepadButtons::B),27,bits);

        const bool left=(bits&mask(GamepadButtons::X))!=0;
        const bool right=(bits&mask(GamepadButtons::Y))!=0;
        if(left!=m_left){SendPointerButton(0,left);m_left=left;}
        if(right!=m_right){SendPointerButton(2,right);m_right=right;}

        constexpr double deadzone=0.18;
        const auto scale=[deadzone](double v)
        {
            const double mag=std::abs(v);
            if(mag<=deadzone)return 0;
            const double n=(mag-deadzone)/(1.0-deadzone);
            return static_cast<int>(std::lround(std::copysign(n*14.0,v)));
        };
        const int dx=scale(reading.LeftThumbstickX), dy=-scale(reading.LeftThumbstickY);
        if(dx||dy)
        {
            if(!m_host.SendPointerRelative(dx,dy))
                m_gamepadX=std::clamp(m_gamepadX+dx,0.0f,m_surfaceWidth-1.0f); m_gamepadY=std::clamp(m_gamepadY+dy,0.0f,m_surfaceHeight-1.0f); m_host.SendPointerAbsolute(static_cast<int>(m_gamepadX),static_cast<int>(m_gamepadY),static_cast<int>(m_surfaceWidth),static_cast<int>(m_surfaceHeight));
            m_host.WakeMainLoop();
        }
        m_previousButtons=bits;
    }

    void QemuInputBridge::ReleaseAll() noexcept
    {
        const auto release=[this](GamepadButtons b,int key)
        {
            if(m_previousButtons&static_cast<std::uint32_t>(b))m_host.SendKeyNumber(key,false);
        };
        release(GamepadButtons::DPadUp,273); release(GamepadButtons::DPadDown,274);
        release(GamepadButtons::DPadLeft,276); release(GamepadButtons::DPadRight,275);
        release(GamepadButtons::A,13); release(GamepadButtons::B,27);
        if(m_left)SendPointerButton(0,false);
        if(m_right)SendPointerButton(2,false);
        if(m_middle)SendPointerButton(2,false);
        m_left=m_right=m_middle=false; m_previousButtons=0;
    }
}
