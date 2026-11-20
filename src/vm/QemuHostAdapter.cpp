#include "QemuHostAdapter.h"
#include "../OnyxNativeLog.h"
#include <array>
#include <chrono>
#include <thread>

namespace onyx::vm
{
    QemuHostAdapter::~QemuHostAdapter() { Stop(); Unload(); }

    bool QemuHostAdapter::Load(const std::wstring& dllName)
    {
        Stop();
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
        const std::array<const char*, 10> required{
            "qemu_host_init", "qemu_host_main_loop_step", "qemu_host_request_shutdown",
            "qemu_host_join", "qemu_host_send_key_number", "qemu_host_send_key_qcode",
            "qemu_host_send_pointer_abs", "qemu_host_send_pointer_button",
            "qemu_host_is_initialized", "qemu_host_is_running"
        };
        for (auto name : required)
            if (!GetProcAddress(m_module, name)) m_missing.emplace_back(name);

        // Match the upstream QEMU-UWP-Host ABI: V2 uses register_video_callback;
        // legacy uses set_video_callback. Do not call the undocumented update variant.
        m_videoRegister = reinterpret_cast<RegisterVideoFn>(GetProcAddress(m_module, "qemu_host_register_video_callback"));
        m_videoSet = reinterpret_cast<SetVideoFn>(GetProcAddress(m_module, "qemu_host_set_video_callback"));
        if (!m_videoRegister && !m_videoSet)
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

        m_init = reinterpret_cast<InitFn>(GetProcAddress(m_module, "qemu_host_init"));
        m_step = reinterpret_cast<MainLoopStepFn>(GetProcAddress(m_module, "qemu_host_main_loop_step"));
        m_shutdown = reinterpret_cast<ShutdownFn>(GetProcAddress(m_module, "qemu_host_request_shutdown"));
        m_join = reinterpret_cast<JoinFn>(GetProcAddress(m_module, "qemu_host_join"));
        m_keyNumber = reinterpret_cast<KeyFn>(GetProcAddress(m_module, "qemu_host_send_key_number"));
        m_keyQcode = reinterpret_cast<KeyFn>(GetProcAddress(m_module, "qemu_host_send_key_qcode"));
        m_pointerRel = reinterpret_cast<PointerRelFn>(GetProcAddress(m_module, "qemu_host_send_pointer_rel"));
        m_pointerAbs = reinterpret_cast<PointerAbsFn>(GetProcAddress(m_module, "qemu_host_send_pointer_abs"));
        m_pointerButton = reinterpret_cast<PointerButtonFn>(GetProcAddress(m_module, "qemu_host_send_pointer_button"));
        m_wake = reinterpret_cast<WakeFn>(GetProcAddress(m_module, "qemu_host_wake_main_loop"));
        m_compatible = true;
        OnyxOS::WriteQemuLogNative(L"QEMU host DLL loaded; init and main-loop exports resolved.");
        return true;
    }

    bool QemuHostAdapter::Start(const std::vector<std::string>& arguments, VideoFrameCallback onVideoFrame)
    {
        if (!m_compatible || m_running || m_worker.joinable() || arguments.empty())
        {
            m_error = L"QEMU não pode iniciar: DLL não carregada, execução ativa ou argumentos vazios.";
            return false;
        }
        m_videoCallback = std::move(onVideoFrame);
        if (m_videoRegister)
            m_videoRegister(&QemuHostAdapter::VideoThunk, this);
        else if (m_videoSet)
            m_videoSet(&QemuHostAdapter::VideoThunk, this);
        m_stopRequested = false;
        try { m_worker = std::thread(&QemuHostAdapter::Run, this, arguments); }
        catch (...) { m_error = L"Não foi possível criar a thread do QEMU."; return false; }
        return true;
    }

    void QemuHostAdapter::Run(std::vector<std::string> arguments) noexcept
    {
        // qemu_host_init follows the normal QEMU argv contract: argv[0] is
        // the executable name, followed by the command-line options.
        if (arguments.empty() || arguments.front() != "qemu-system-x86_64")
            arguments.insert(arguments.begin(), "qemu-system-x86_64");
        std::vector<const char*> argv;
        argv.reserve(arguments.size());
        for (const auto& arg : arguments) argv.push_back(arg.c_str());
        m_running = true;
        OnyxOS::WriteQemuLogNative(L"Calling qemu_host_init.");
        const int initResult = m_init(static_cast<int>(argv.size()), argv.data());
        if (initResult != 0)
        {
            m_error = L"qemu_host_init falhou: " + std::to_wstring(initResult);
            OnyxOS::WriteQemuLogNative(m_error.c_str());
            m_running = false;
            return;
        }
        OnyxOS::WriteQemuLogNative(L"QEMU initialized; entering main loop.");
        int status = 0;
        while (!m_stopRequested)
        {
            const int result = m_step(m_wake == nullptr, &status);
            if (result != 0)
            {
                OnyxOS::WriteQemuLogNative((L"QEMU main loop ended: " + std::to_wstring(result) + L", status " + std::to_wstring(status)).c_str());
                break;
            }
            if (m_wake == nullptr) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (m_join) { int finalStatus = 0; m_join(&finalStatus); }
        m_running = false;
        OnyxOS::WriteQemuLogNative(L"QEMU main loop stopped.");
    }

    void QemuHostAdapter::Stop() noexcept
    {
        if (m_worker.joinable())
        {
            m_stopRequested = true;
            if (m_shutdown) m_shutdown();
            if (m_wake) m_wake();
            m_worker.join();
        }
        m_running = false;
    }

    void QemuHostAdapter::VideoThunk(void* opaque, const void* pixels, int width, int height, int stride, int format)
    {
        auto self = static_cast<QemuHostAdapter*>(opaque);
        if (self && self->m_videoCallback) self->m_videoCallback(pixels, width, height, stride, format);
    }

    void QemuHostAdapter::Unload() noexcept
    {
        if (m_module) FreeLibrary(m_module);
        m_module = nullptr; m_compatible = false; m_apiVersion = 0;
        m_init = nullptr; m_step = nullptr; m_shutdown = nullptr; m_join = nullptr;
        m_videoRegister = nullptr; m_videoSet = nullptr;
        m_keyNumber = nullptr; m_keyQcode = nullptr; m_pointerRel = nullptr;
        m_pointerAbs = nullptr; m_pointerButton = nullptr; m_wake = nullptr;
        m_videoCallback = {};
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
