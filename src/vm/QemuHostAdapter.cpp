#include "QemuHostAdapter.h"

#include <array>
#include <sstream>

namespace onyx::vm
{
    namespace
    {
        using GetApiVersionFn = std::uint32_t(__cdecl*)();

        constexpr std::array<const char*, 15> RequiredExports{
            "qemu_host_init",
            "qemu_host_start",
            "qemu_host_main_loop_step",
            "qemu_host_request_shutdown",
            "qemu_host_reset",
            "qemu_host_join",
            "qemu_host_cleanup",
            "qemu_host_send_key_number",
            "qemu_host_send_key_qcode",
            "qemu_host_send_pointer_abs",
            "qemu_host_send_pointer_button",
            "qemu_host_is_initialized",
            "qemu_host_is_running",
            "qemu_host_register_video_callback",
            "qemu_host_set_video_callback"
        };
    }

    QemuHostAdapter::~QemuHostAdapter()
    {
        Unload();
    }

    bool QemuHostAdapter::Load(const std::wstring& dllName)
    {
        Unload();
        m_error.clear();
        m_missing.clear();

        m_module = LoadPackagedLibrary(dllName.c_str(), 0);
        if (!m_module)
        {
            const DWORD error = GetLastError();
            m_error = L"LoadPackagedLibrary falhou. Código Win32: " + std::to_wstring(error);
            return false;
        }

        const auto versionFn = reinterpret_cast<GetApiVersionFn>(
            GetProcAddress(m_module, "qemu_host_get_api_version"));

        // Validate the stable lifecycle/input ABI names before declaring the DLL usable.
        const std::array<const char*, 13> mandatory{
            "qemu_host_init", "qemu_host_start", "qemu_host_main_loop_step",
            "qemu_host_request_shutdown", "qemu_host_reset", "qemu_host_join",
            "qemu_host_cleanup", "qemu_host_send_key_number", "qemu_host_send_key_qcode",
            "qemu_host_send_pointer_abs", "qemu_host_send_pointer_button",
            "qemu_host_is_initialized", "qemu_host_is_running"
        };

        for (const char* name : mandatory)
        {
            if (!GetProcAddress(m_module, name))
                m_missing.emplace_back(name);
        }

        const bool hasVideoCallback =
            GetProcAddress(m_module, "qemu_host_register_video_callback") != nullptr ||
            GetProcAddress(m_module, "qemu_host_set_video_callback") != nullptr;

        if (!hasVideoCallback)
            m_missing.emplace_back("qemu_host_register_video_callback OR qemu_host_set_video_callback");

        if (!m_missing.empty())
        {
            m_error = L"A DLL foi carregada, mas não corresponde à API de host QEMU esperada.";
            Unload();
            return false;
        }

        if (versionFn)
        {
            m_apiVersion = versionFn();
            const std::uint32_t major = m_apiVersion >> 16;
            if (major != 1)
            {
                m_error = L"Versão incompatível da API QEMU. Esperada API major 1.";
                Unload();
                return false;
            }
        }
        else
        {
            // Legacy host DLL: exports were checked, but its ABI version is inferred.
            m_apiVersion = 0;
        }

        m_compatible = true;
        return true;
    }

    void QemuHostAdapter::Unload() noexcept
    {
        if (m_module)
        {
            FreeLibrary(m_module);
            m_module = nullptr;
        }
        m_compatible = false;
        m_apiVersion = 0;
    }
}
