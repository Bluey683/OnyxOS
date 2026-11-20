#include "QemuHostAdapter.h"
#include "../OnyxNativeLog.h"
#include <array>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <thread>
#include <roapi.h>

namespace onyx::vm
{
    namespace
    {
        std::uint32_t ExpandColorBits(std::uint32_t value, int bits)
        {
            if (bits <= 0) return 0;
            if (bits >= 8) return value & 0xffu;
            const std::uint32_t maximum = (1u << bits) - 1u;
            return (value * 255u + maximum / 2u) / maximum;
        }
        std::uint32_t PackBgra(std::uint32_t r, std::uint32_t g, std::uint32_t b)
        { return 0xff000000u | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu); }
        bool ConvertQemuFrame(const void* pixels, int width, int height, int stride, int format, std::vector<std::uint32_t>& output)
        {
            if (!pixels || width <= 0 || height <= 0 || stride <= 0 ||
                static_cast<std::size_t>(width) > (SIZE_MAX / sizeof(std::uint32_t)) / static_cast<std::size_t>(height)) return false;
            output.resize(static_cast<std::size_t>(width) * height);
            if (format >= 1 && format <= 4) {
                if (static_cast<std::size_t>(stride) < static_cast<std::size_t>(width) * 4) return false;
                for (int y=0;y<height;++y) { const auto* row=static_cast<const std::uint8_t*>(pixels)+static_cast<std::size_t>(y)*stride;
                    for(int x=0;x<width;++x){const auto* p=row+static_cast<std::size_t>(x)*4;const bool bgr=format==1||format==4;
                        output[static_cast<std::size_t>(y)*width+x]=PackBgra(bgr?p[2]:p[0],p[1],bgr?p[0]:p[2]);}}
                return true;
            }
            const int bpp=(format>>24)&0xff, type=(format>>16)&0xff, rb=(format>>8)&0xf, gb=(format>>4)&0xf, bb=format&0xf;
            const bool reversed=type==3||type==8||type==9; const int bytes=(bpp+7)/8;
            if((bpp!=32&&bpp!=24&&bpp!=16&&bpp!=15&&bpp!=8)||static_cast<std::size_t>(stride)<static_cast<std::size_t>(width)*bytes)return false;
            for(int y=0;y<height;++y){const auto* row=static_cast<const std::uint8_t*>(pixels)+static_cast<std::size_t>(y)*stride;
                for(int x=0;x<width;++x){std::uint32_t r=0,g=0,b=0;
                    if(bpp==32){std::uint32_t v;std::memcpy(&v,row+static_cast<std::size_t>(x)*4,4);r=reversed?(v&255u):((v>>16)&255u);g=(v>>8)&255u;b=reversed?((v>>16)&255u):(v&255u);}
                    else if(bpp==24){const auto* p=row+static_cast<std::size_t>(x)*3;r=reversed?p[0]:p[2];g=p[1];b=reversed?p[2]:p[0];}
                    else if(bpp==16||bpp==15){std::uint16_t v;std::memcpy(&v,row+static_cast<std::size_t>(x)*2,2);const std::uint32_t bm=bb?((1u<<bb)-1u):0u,gm=gb?((1u<<gb)-1u):0u,rm=rb?((1u<<rb)-1u):0u;b=ExpandColorBits(v&bm,bb);g=ExpandColorBits((v>>bb)&gm,gb);r=ExpandColorBits((v>>(bb+gb))&rm,rb);if(reversed)std::swap(r,b);}
                    else {r=g=b=row[x];}
                    output[static_cast<std::size_t>(y)*width+x]=PackBgra(r,g,b);
                }} return true;
        }
    }
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
        m_logRegister = reinterpret_cast<RegisterLogV2Fn>(GetProcAddress(m_module, "qemu_host_register_log_callback"));
        m_logSet = reinterpret_cast<SetLogLegacyFn>(GetProcAddress(m_module, "qemu_host_set_log_callback"));
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
        {
            std::lock_guard<std::mutex> lock(m_startMutex);
            m_startComplete = false;
            m_startSucceeded = false;
            m_firstFrameReceived = false;
        }
        if (m_logRegister) {
            m_logRegister(&QemuHostAdapter::LogThunkV2, this);
            OnyxOS::WriteQemuLogNative(L"Registered QEMU v2 native log callback.");
        } else if (m_logSet) {
            m_logSet(&QemuHostAdapter::LogThunkLegacy, this);
            OnyxOS::WriteQemuLogNative(L"Registered legacy QEMU native log callback.");
        } else {
            OnyxOS::WriteQemuLogNative(L"WARNING: QEMU DLL has no native log callback export.");
        }
        if (m_videoRegister)
            m_videoRegister(&QemuHostAdapter::VideoThunk, this);
        else if (m_videoSet)
            m_videoSet(&QemuHostAdapter::LegacyVideoThunk, this);
        m_stopRequested = false;
        try { m_worker = std::thread(&QemuHostAdapter::Run, this, arguments); }
        catch (...) { m_error = L"Não foi possível criar a thread do QEMU."; return false; }
        std::unique_lock<std::mutex> lock(m_startMutex);
        if (!m_startCondition.wait_for(lock, std::chrono::seconds(20),
            [this] { return m_startComplete; }))
        {
            m_error = L"Tempo esgotado aguardando qemu_host_init (20 segundos).";
            lock.unlock();
            Stop();
            return false;
        }
        const bool initialized = m_startSucceeded;
        if (!initialized)
        {
            lock.unlock();
            if (m_worker.joinable()) m_worker.join();
            return false;
        }
        // Wait for a real guest frame before reporting the VM as started.
        const bool gotFirstFrame = m_startCondition.wait_for(lock, std::chrono::seconds(45),
            [this] { return m_firstFrameReceived || !m_running.load(); }) && m_firstFrameReceived;
        lock.unlock();
        if (!gotFirstFrame)
        {
            m_error = L"QEMU inicializou, mas não entregou o primeiro framebuffer em 45 segundos. Verifique BIOS/firmware, -display host e o log QEMU.";
            OnyxOS::WriteQemuLogNative(m_error.c_str());
            Stop();
            return false;
        }
        return true;
    }

    void QemuHostAdapter::Run(std::vector<std::string> arguments) noexcept
    {
        // The upstream UWP host runs qemu_host_init on a WinRT ThreadPool
        // worker. A plain std::thread has no WinRT apartment; initialize MTA
        // explicitly before entering QEMU, which may call WinRT/UWP APIs.
        const HRESULT apartmentResult = RoInitialize(RO_INIT_MULTITHREADED);
        if (FAILED(apartmentResult))
        {
            m_error = L"RoInitialize(MTA) falhou antes do QEMU: HRESULT " +
                std::to_wstring(static_cast<unsigned long>(apartmentResult));
            OnyxOS::WriteQemuLogNative(m_error.c_str());
            {
                std::lock_guard<std::mutex> lock(m_startMutex);
                m_startComplete = true;
                m_startSucceeded = false;
            }
            m_startCondition.notify_all();
            m_running = false;
            return;
        }
        OnyxOS::WriteQemuLogNative(L"WinRT MTA apartment initialized for QEMU worker.");
        // qemu_host_init follows the normal QEMU argv contract: argv[0] is
        // the executable name, followed by the command-line options.
        if (arguments.empty() || arguments.front() != "qemu-system-x86_64")
            arguments.insert(arguments.begin(), "qemu-system-x86_64");
        std::vector<const char*> argv;
        argv.reserve(arguments.size());
        for (const auto& arg : arguments) argv.push_back(arg.c_str());
        m_running = true;
        OnyxOS::WriteQemuLogNative(L"QEMU argv:");
        for (const auto& arg : arguments) {
            const int count = MultiByteToWideChar(CP_UTF8, 0, arg.c_str(), -1, nullptr, 0);
            if (count > 1) { std::wstring wide(static_cast<std::size_t>(count), L"\\0"[0]); MultiByteToWideChar(CP_UTF8, 0, arg.c_str(), -1, wide.data(), count); wide.resize(static_cast<std::size_t>(count - 1)); OnyxOS::WriteQemuLogNative(wide.c_str()); }
        }
        OnyxOS::WriteQemuLogNative(L"Calling qemu_host_init.");
        const int initResult = m_init(static_cast<int>(argv.size()), argv.data());
        if (initResult != 0)
        {
            m_error = L"qemu_host_init falhou: " + std::to_wstring(initResult);
            OnyxOS::WriteQemuLogNative(m_error.c_str());
            m_running = false;
            {
                std::lock_guard<std::mutex> lock(m_startMutex);
                m_startComplete = true;
                m_startSucceeded = false;
            }
            m_startCondition.notify_all();
            return;
        }
        {
            std::lock_guard<std::mutex> lock(m_startMutex);
            m_startComplete = true;
            m_startSucceeded = true;
        }
        m_startCondition.notify_all();
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
        m_startCondition.notify_all();
        OnyxOS::WriteQemuLogNative(L"QEMU main loop stopped.");
        RoUninitialize();
        OnyxOS::WriteQemuLogNative(L"WinRT MTA apartment released.");
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

    void QemuHostAdapter::LogThunkV2(void*, int level, const char* message)
    {
        if (!message) return;
        const int count = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
        if (count <= 1) return;
        std::wstring wide(static_cast<std::size_t>(count), L"\\0"[0]);
        MultiByteToWideChar(CP_UTF8, 0, message, -1, wide.data(), count);
        wide.resize(static_cast<std::size_t>(count - 1));
        OnyxOS::WriteQemuLogNative((L"[QEMU native level " + std::to_wstring(level) + L"] " + wide).c_str());
    }

    void QemuHostAdapter::LogThunkLegacy(int level, const char* message, void* opaque)
    {
        LogThunkV2(opaque, level, message);
    }

    void QemuHostAdapter::VideoThunk(void* opaque, const void* pixels, int width, int height, int stride, int format)
    {
        auto self = static_cast<QemuHostAdapter*>(opaque);
        if (!self || !self->m_videoCallback) return;
        std::vector<std::uint32_t> bgra;
        if (!ConvertQemuFrame(pixels, width, height, stride, format, bgra)) {
            OnyxOS::WriteQemuLogNative(L"Unsupported or invalid QEMU framebuffer format/stride.");
            return;
        }
        self->m_videoCallback(bgra, width, height);
        {
            std::lock_guard<std::mutex> lock(self->m_startMutex);
            self->m_firstFrameReceived = true;
        }
        self->m_startCondition.notify_all();
    }

    void QemuHostAdapter::LegacyVideoThunk(const void* pixels, int width, int height, int stride, int format, void* opaque)
    {
        VideoThunk(opaque, pixels, width, height, stride, format);
    }

    void QemuHostAdapter::Unload() noexcept
    {
        if (m_module) FreeLibrary(m_module);
        m_module = nullptr; m_compatible = false; m_apiVersion = 0;
        m_init = nullptr; m_step = nullptr; m_shutdown = nullptr; m_join = nullptr;
        m_videoRegister = nullptr; m_videoSet = nullptr;
        m_logRegister = nullptr; m_logSet = nullptr;
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
