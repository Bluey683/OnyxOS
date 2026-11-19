#include "QemuHostAdapter.h"
#include "../OnyxNativeLog.h"
#include <array>

namespace onyx::vm
{
    QemuHostAdapter::~QemuHostAdapter() { Unload(); }

    bool QemuHostAdapter::Load(const std::wstring& dllName)
    {
        Unload();
        OnyxOS::WriteQemuLogNative(L"Loading packaged QEMU host DLL.");
        m_error.clear();
        m_missing.clear();
        m_module = LoadPackagedLibrary(dllName.c_str(), 0);
        if (!m_module)
        {
            m_error = L"LoadPackagedLibrary falhou. Código Win32: " + std::to_wstring(GetLastError());
            OnyxOS::WriteQemuLogNative(m_error.c_str());
            return false;
        }

        using VersionFn = std::uint32_t(*)();
        auto version = reinterpret_cast<VersionFn>(GetProcAddress(m_module, "qemu_host_get_api_version"));
        const std::array<const char*, 13> required{
            "qemu_host_init", "qemu_host_start", "qemu_host_main_loop_step",
            "qemu_host_request_shutdown", "qemu_host_reset", "qemu_host_join",
            "qemu_host_cleanup", "qemu_host_send_key_number", "qemu_host_send_key_qcode",
            "qemu_host_send_pointer_abs", "qemu_host_send_pointer_button",
            "qemu_host_is_initialized", "qemu_host_is_running"
        };
        for (auto name : required)
            if (!GetProcAddress(m_module, name)) m_missing.emplace_back(name);

        if (!GetProcAddress(m_module, "qemu_host_register_video_callback") &&
            !GetProcAddress(m_module, "qemu_host_set_video_callback") &&
            !GetProcAddress(m_module, "qemu_host_register_video_update_callback"))
            m_missing.emplace_back("QEMU video callback registration");

        if (!m_missing.empty())
        {
            m_error = L"A DLL não corresponde à API QEMU UWP esperada.";
            OnyxOS::WriteQemuLogNative(m_error.c_str());
            for (const auto& missing : m_missing)
                OnyxOS::WriteQemuLogNative(std::wstring(missing.begin(), missing.end()).c_str());
            Unload();
            return false;
        }
        if (version)
        {
            m_apiVersion = version();
            if ((m_apiVersion >> 16) != 1)
            {
                m_error = L"Versão incompatível da API QEMU. Esperada major 1.";
                OnyxOS::WriteQemuLogNative(m_error.c_str());
                Unload();
                return false;
            }
        }

        m_keyNumber = reinterpret_cast<KeyFn>(GetProcAddress(m_module, "qemu_host_send_key_number"));
        m_keyQcode = reinterpret_cast<KeyFn>(GetProcAddress(m_module, "qemu_host_send_key_qcode"));
        m_pointerRel = reinterpret_cast<PointerRelFn>(GetProcAddress(m_module, "qemu_host_send_pointer_rel"));
        m_pointerAbs = reinterpret_cast<PointerAbsFn>(GetProcAddress(m_module, "qemu_host_send_pointer_abs"));
        m_pointerButton = reinterpret_cast<PointerButtonFn>(GetProcAddress(m_module, "qemu_host_send_pointer_button"));
        m_wake = reinterpret_cast<WakeFn>(GetProcAddress(m_module, "qemu_host_wake_main_loop"));
        m_compatible = true;
        OnyxOS::WriteQemuLogNative(L"QEMU host DLL loaded and required exports validated.");
        return true;
    }

    void QemuHostAdapter::Unload() noexcept
    {
        if (m_module) FreeLibrary(m_module);
        m_module = nullptr; m_compatible = false; m_apiVersion = 0;
        m_keyNumber = nullptr; m_keyQcode = nullptr; m_pointerRel = nullptr;
        m_pointerAbs = nullptr; m_pointerButton = nullptr; m_wake = nullptr;
    }

    bool QemuHostAdapter::SendKeyNumber(int key, bool down) const noexcept
    { return m_compatible && m_keyNumber && m_keyNumber(key, down) == 0; }
    bool QemuHostAdapter::SendKeyQcode(int key, bool down) const noexcept
    { return m_compatible && m_keyQcode && m_keyQcode(key, down) == 0; }
    bool QemuHostAdapter::SendPointerRelative(int x, int y) const noexcept
    { return m_compatible && m_pointerRel && m_pointerRel(x, y) == 0; }
    bool QemuHostAdapter::SendPointerAbsolute(int x, int y, int w, int h) const noexcept
    { return m_compatible && m_pointerAbs && m_pointerAbs(x, y, w, h) == 0; }
    bool QemuHostAdapter::SendPointerButton(int button, bool down) const noexcept
    { return m_compatible && m_pointerButton && m_pointerButton(button, down) == 0; }
    bool QemuHostAdapter::WakeMainLoop() const noexcept
    {
        if (!m_compatible || !m_wake) return false;
        m_wake();
        return true;
    }
}
