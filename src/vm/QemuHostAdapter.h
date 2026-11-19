#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

namespace onyx::vm
{
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

        bool SendKeyNumber(int keycode, bool down) const noexcept;
        bool SendKeyQcode(int qcode, bool down) const noexcept;
        bool SendPointerRelative(int dx, int dy) const noexcept;
        bool SendPointerAbsolute(int x, int y, int width, int height) const noexcept;
        bool SendPointerButton(int button, bool down) const noexcept;
        bool WakeMainLoop() const noexcept;

    private:
        using KeyFn = int(*)(int, bool);
        using PointerRelFn = int(*)(int, int);
        using PointerAbsFn = int(*)(int, int, int, int);
        using PointerButtonFn = int(*)(int, bool);
        using WakeFn = void(*)();

        HMODULE m_module{ nullptr };
        bool m_compatible{ false };
        std::uint32_t m_apiVersion{ 0 };
        std::wstring m_error;
        std::vector<std::string> m_missing;
        KeyFn m_keyNumber{ nullptr }, m_keyQcode{ nullptr };
        PointerRelFn m_pointerRel{ nullptr };
        PointerAbsFn m_pointerAbs{ nullptr };
        PointerButtonFn m_pointerButton{ nullptr };
        WakeFn m_wake{ nullptr };
    };
}
