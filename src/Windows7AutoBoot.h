#pragma once
#include "vm/QemuHostAdapter.h"
#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace onyx::app {
class Windows7AutoBoot final {
public:
    using FrameCallback = onyx::vm::QemuHostAdapter::VideoFrameCallback;
    explicit Windows7AutoBoot(FrameCallback callback);
    ~Windows7AutoBoot();
    Windows7AutoBoot(const Windows7AutoBoot&) = delete;
    Windows7AutoBoot& operator=(const Windows7AutoBoot&) = delete;
    void Start();
    void Stop() noexcept;
private:
    void Run() noexcept;
    FrameCallback m_frameCallback;
    onyx::vm::QemuHostAdapter m_qemu;
    std::thread m_worker;
    std::atomic<bool> m_stopRequested{false};
};
}
