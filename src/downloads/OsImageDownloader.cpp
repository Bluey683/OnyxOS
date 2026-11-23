#include "OsImageDownloader.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.h>
#include <winrt/Windows.Networking.BackgroundTransfer.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winerror.h>
#include <array>
#include <cstdint>

namespace wf = winrt::Windows::Foundation;
namespace wnbt = winrt::Windows::Networking::BackgroundTransfer;
namespace ws = winrt::Windows::Storage;
namespace wss = winrt::Windows::Storage::Streams;

namespace onyx::downloads {
namespace {
const std::array<OsImageSource, 5> Sources{{
    {"debian", L"debian.iso", L"https://cloudfront.debian.net/debian-cd/current/amd64/iso-cd/debian-13.7.0-amd64-netinst.iso", L"https://www.debian.org/download", ImageSourceKind::DirectIso},
    {"ubuntu", L"ubuntu.iso", L"https://releases.ubuntu.com/26.04/ubuntu-26.04.1-desktop-amd64.iso", L"https://ubuntu.com/download/desktop", ImageSourceKind::DirectIso},
    {"arch-linux", L"archlinux.iso", L"https://geo.mirror.pkgbuild.com/iso/latest/archlinux-x86_64.iso", L"https://archlinux.org/download/", ImageSourceKind::DirectIso},
    {"windows-10", L"windows-10.iso", L"", L"https://www.microsoft.com/software-download/windows10ISO", ImageSourceKind::MicrosoftDownloadPage},
    {"windows-11", L"windows-11.iso", L"", L"https://www.microsoft.com/software-download/windows11", ImageSourceKind::MicrosoftDownloadPage}
}};

constexpr wchar_t Windows7Qcow2Url[] =
    L"https://archive.org/download/windows-image-collection/Windows%207.qcow2";
constexpr wchar_t Windows7Qcow2Name[] = L"windows7.qcow2";

wf::IAsyncOperation<bool> HasQcow2MagicAsync(ws::StorageFile file) {
    auto stream = co_await file.OpenAsync(ws::FileAccessMode::Read);
    wss::DataReader reader(stream);
    const auto loaded = co_await reader.LoadAsync(4);
    if (loaded != 4) {
        reader.Close();
        stream.Close();
        co_return false;
    }
    std::array<std::uint8_t, 4> magic{};
    reader.ReadBytes(magic);
    reader.Close();
    stream.Close();
    co_return magic[0] == 'Q' && magic[1] == 'F' &&
              magic[2] == 'I' && magic[3] == 0xfb;
}
}

const OsImageSource* FindImageSource(const std::string& id) noexcept {
    for (const auto& source : Sources)
        if (source.profileId == id) return &source;
    return nullptr;
}

wf::IAsyncOperation<ws::StorageFile> OsImageDownloader::DownloadOfficialIsoAsync(
    const std::string& id, ws::StorageFolder folder) {
    const auto* source = FindImageSource(id);
    if (!source)
        throw winrt::hresult_invalid_argument(L"No official ISO source for this profile.");
    if (source->kind != ImageSourceKind::DirectIso || source->url.empty())
        throw winrt::hresult_error(E_NOTIMPL,
            L"Microsoft requires edition/language selection on its official download page. Open the page and select the ISO.");

    auto file = co_await folder.CreateFileAsync(
        winrt::hstring(source->fileName), ws::CreationCollisionOption::ReplaceExisting);
    wnbt::BackgroundDownloader downloader;
    auto operation = downloader.CreateDownload(wf::Uri(winrt::hstring(source->url)), file);
    co_await operation.StartAsync();

    auto properties = co_await file.GetBasicPropertiesAsync();
    if (properties.Size() < 65536) {
        co_await file.DeleteAsync();
        throw winrt::hresult_error(E_FAIL, L"Downloaded file is too small to be a valid installer ISO.");
    }
    co_return file;
}

wf::IAsyncOperation<ws::StorageFile> OsImageDownloader::DownloadWindows7Qcow2Async(
    ws::StorageFolder destination) {
    if (!destination)
        throw winrt::hresult_invalid_argument(L"Destination folder is required.");

    // A previous failed transfer may have left an empty file behind. Always
    // remove it before starting a new transfer, and never preserve a partial
    // QCOW2 as if it were a valid guest disk.
    ws::StorageFile file{ nullptr };
    try {
        file = co_await destination.CreateFileAsync(
            Windows7Qcow2Name, ws::CreationCollisionOption::ReplaceExisting);
    } catch (...) {
        throw;
    }

    try {
        wnbt::BackgroundDownloader downloader;
        auto request = wf::Uri(L"https://archive.org/download/windows-image-collection/Windows%207.qcow2?download=1");
        auto operation = downloader.CreateDownload(request, file);
        co_await operation.StartAsync();

        const auto properties = co_await file.GetBasicPropertiesAsync();
        if (properties.Size() < 1024 * 1024) {
            throw winrt::hresult_error(E_FAIL,
                L"Archive.org terminou a transferência sem entregar o disco (arquivo menor que 1 MiB).");
        }

        if (!(co_await HasQcow2MagicAsync(file))) {
            throw winrt::hresult_error(E_FAIL,
                L"O download não é um disco QCOW2 válido (cabeçalho QFI ausente).");
        }
        co_return file;
    } catch (...) {
        try {
            if (file) file.DeleteAsync().get();
        } catch (...) {
            // Preserve the original network/validation error.
        }
        throw;
    }
}

wf::IAsyncOperation<ws::StorageFile> OsImageDownloader::ImportWindows7Qcow2Async(
    ws::StorageFile source, ws::StorageFolder destination) {
    if (!source || !destination)
        throw winrt::hresult_invalid_argument(L"Source file and destination folder are required.");

    auto imported = co_await source.CopyAsync(
        destination, Windows7Qcow2Name, ws::NameCollisionOption::ReplaceExisting);
    if (!(co_await HasQcow2MagicAsync(imported))) {
        co_await imported.DeleteAsync();
        throw winrt::hresult_error(E_FAIL,
            L"The selected file is not a valid QCOW2 disk image (missing QFI magic).");
    }
    co_return imported;
}
}
