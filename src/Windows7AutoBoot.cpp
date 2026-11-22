#include "Windows7AutoBoot.h"
#include "vm/VirtualMachine.h"
#include "downloads/OsImageDownloader.h"
#include "OnyxNativeLog.h"
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winerror.h>

namespace ws = winrt::Windows::Storage;
namespace appdata = winrt::Windows::ApplicationModel;

namespace onyx::app {
Windows7AutoBoot::Windows7AutoBoot(FrameCallback callback)
    : m_frameCallback(std::move(callback)) {}

Windows7AutoBoot::~Windows7AutoBoot() { Stop(); }

void Windows7AutoBoot::Start() {
    if (m_worker.joinable()) return;
    m_stopRequested = false;
    m_worker = std::thread(&Windows7AutoBoot::Run, this);
}

void Windows7AutoBoot::Stop() noexcept {
    m_stopRequested = true;
    m_qemu.Stop();
    if (m_worker.joinable() && m_worker.get_id() != std::this_thread::get_id())
        m_worker.join();
}

void Windows7AutoBoot::Run() noexcept {
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        OnyxOS::WriteQemuLogNative(L"Windows 7 auto-boot: checking app-private disk.");
        auto local = appdata::ApplicationData::Current().LocalFolder();
        auto disks = local.CreateFolderAsync(L"Disks", ws::CreationCollisionOption::OpenIfExists).get();
        auto item = disks.TryGetItemAsync(L"windows7.qcow2").get();
        ws::StorageFile disk = item ? item.try_as<ws::StorageFile>() : nullptr;

        if (!disk) {
            OnyxOS::WriteQemuLogNative(L"Windows 7 QCOW2 not found. Downloading from Archive.org...");
            disk = onyx::downloads::OsImageDownloader::DownloadWindows7Qcow2Async(disks).get();
            OnyxOS::WriteQemuLogNative(L"Windows 7 QCOW2 download and QFI validation completed.");
        } else {
            OnyxOS::WriteQemuLogNative(L"Existing Windows 7 QCOW2 found; reusing LocalState copy.");
        }

        if (m_stopRequested) return;
        auto config = onyx::vm::CreateProfile("windows7");
        config.diskPath = winrt::to_string(disk.Path());
        config.diskFormat = "qcow2";
        config.enableAudio = false;
        const auto validation = onyx::vm::Validate(config);
        if (!validation.valid)
            throw winrt::hresult_error(E_INVALIDARG, winrt::to_hstring(validation.message));

        if (!m_qemu.Load())
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                winrt::hstring(m_qemu.Error()));
        auto arguments = onyx::vm::BuildQemuArguments(config);
        if (arguments.empty())
            throw winrt::hresult_error(E_INVALIDARG, L"QEMU argument list is empty.");

        OnyxOS::WriteQemuLogNative(L"Starting Windows 7 x64 automatically.");
        if (!m_qemu.Start(arguments, m_frameCallback)) {
            const auto message = m_qemu.Error().empty()
                ? std::wstring(L"QEMU failed to start Windows 7.")
                : m_qemu.Error();
            OnyxOS::WriteQemuLogNative(message.c_str());
            return;
        }
        OnyxOS::WriteQemuLogNative(L"Windows 7 guest delivered its first video frame.");
    } catch (const winrt::hresult_error& error) {
        const auto message = std::wstring(L"Windows 7 auto-boot failed: ") + error.message().c_str();
        OnyxOS::WriteQemuLogNative(message.c_str());
    } catch (const std::exception& error) {
        const auto message = std::wstring(L"Windows 7 auto-boot failed (native exception): ") +
            std::wstring(error.what(), error.what() + strlen(error.what()));
        OnyxOS::WriteQemuLogNative(message.c_str());
    } catch (...) {
        OnyxOS::WriteQemuLogNative(L"Windows 7 auto-boot failed with an unknown exception.");
    }
    try { winrt::uninit_apartment(); } catch (...) {}
}
}
