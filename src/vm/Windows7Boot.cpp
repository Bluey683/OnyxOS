#include "Windows7Boot.h"
#include "../downloads/OsImageDownloader.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>
#include <array>
#include <windows.h>
#include <cstdint>
#include <exception>

namespace ws = winrt::Windows::Storage;
namespace wss = winrt::Windows::Storage::Streams;

namespace onyx::vm {
    bool PrepareWindows7Disk(const std::wstring& localStatePath,
                             std::wstring& diskPath,
                             std::wstring& error) noexcept {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            try {
                auto local = ws::StorageFolder::GetFolderFromPathAsync(localStatePath).get();
                auto disks = local.CreateFolderAsync(
                    L"Disks", ws::CreationCollisionOption::OpenIfExists).get();

                ws::StorageFile disk{ nullptr };
                try {
                    disk = disks.GetFileAsync(L"windows7.qcow2").get();
                } catch (const winrt::hresult_error&) {
                    disk = onyx::downloads::OsImageDownloader::DownloadWindows7Qcow2Async(disks).get();
                }

                auto stream = disk.OpenAsync(ws::FileAccessMode::Read).get();
                wss::DataReader reader(stream);
                const auto count = reader.LoadAsync(4).get();
                std::array<std::uint8_t, 4> magic{};
                if (count == 4) reader.ReadBytes(magic);
                reader.Close();
                stream.Close();
                if (count != 4 || magic[0] != 'Q' || magic[1] != 'F' ||
                    magic[2] != 'I' || magic[3] != 0xfb) {
                    disk.DeleteAsync().get();
                    throw winrt::hresult_error(E_FAIL,
                        L"windows7.qcow2 existe, mas não é um disco QCOW2 válido.");
                }

                diskPath = disk.Path().c_str();
                winrt::uninit_apartment();
                return true;
            } catch (...) {
                winrt::uninit_apartment();
                throw;
            }
        } catch (const winrt::hresult_error& ex) {
            error = ex.message().c_str();
        } catch (const std::exception& ex) {
            const auto length = MultiByteToWideChar(CP_UTF8, 0, ex.what(), -1, nullptr, 0);
            if (length > 1) {
                error.resize(static_cast<std::size_t>(length));
                MultiByteToWideChar(CP_UTF8, 0, ex.what(), -1, error.data(), length);
                error.resize(static_cast<std::size_t>(length - 1));
            } else error = L"Falha preparando o disco do Windows 7.";
        } catch (...) {
            error = L"Falha desconhecida preparando o disco do Windows 7.";
        }
        return false;
    }
}
