#pragma once
#include "QemuHostAdapter.h"
#include <cstdint>

namespace onyx::vm
{
    class QemuInputBridge final
    {
    public:
        explicit QemuInputBridge(QemuHostAdapter& host) noexcept : m_host(host) {}
        void SendVirtualKey(int virtualKey, bool down) noexcept;
        void SendPointerPosition(float x, float y, float width, float height) noexcept;
        void SendPointerButton(int button, bool down) noexcept;
        void PollGamepad() noexcept;
        void ReleaseAll() noexcept;
    private:
        static int MapVirtualKey(int key) noexcept;
        void UpdateGamepadKey(std::uint32_t mask, int key, std::uint32_t current) noexcept;
        QemuHostAdapter& m_host;
        std::uint32_t m_previousButtons{0};
        float m_lastX{0}, m_lastY{0};
        float m_surfaceWidth{1920}, m_surfaceHeight{1080};
        float m_gamepadX{960}, m_gamepadY{540};
        bool m_havePointer{false};
        bool m_left{false}, m_right{false}, m_middle{false};
    };
}
