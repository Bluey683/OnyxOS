#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

namespace onyx::vm
{
    // Loads and validates the packaged QEMU UWP host DLL. This adapter deliberately
    // does not start the guest until the exact callback ABI is integrated.
    class QemuHostAdapter final
    {
    public:
        QemuHostAdapter() = default;
        ~QemuHostAdapter();

        QemuHostAdapter(const QemuHostAdapter&) = delete;
        QemuHostAdapter& operator=(const QemuHostAdapter&) = delete;

        bool Load(const std::wstring& dllName = L"qemu-system-x86_64.dll");
        void Unload() noexcept;

        bool IsLoaded() const noexcept { return m_module != nullptr; }
        bool IsCompatible() const noexcept { return m_compatible; }
        std::uint32_t ApiVersion() const noexcept { return m_apiVersion; }
        const std::wstring& Error() const noexcept { return m_error; }
        const std::vector<std::string>& MissingExports() const noexcept { return m_missing; }

    private:
        HMODULE m_module{ nullptr };
        bool m_compatible{ false };
        std::uint32_t m_apiVersion{ 0 };
        std::wstring m_error;
        std::vector<std::string> m_missing;
    };
}
