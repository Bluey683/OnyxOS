#include "OsImageDownloader.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.h>
#include <winrt/Windows.Networking.BackgroundTransfer.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winerror.h>

namespace wf = winrt::Windows::Foundation;
namespace wnbt = winrt::Windows::Networking::BackgroundTransfer;
namespace ws = winrt::Windows::Storage;

namespace onyx::downloads
{
    namespace
    {
        const OsImageSource Windows11Source{
            "windows11",
            L"Windows11.iso",
            L"",
            L"https://www.microsoft.com/software-download/windows11",
            ImageSourceKind::MicrosoftDownloadPage
        };
    }

    const OsImageSource* FindImageSource(const std::string& id) noexcept
    {
        return id == "windows11" ? &Windows11Source : nullptr;
    }

    wf::IAsyncOperation<ws::StorageFile> OsImageDownloader::DownloadOfficialIsoAsync(
        const std::string& id, ws::StorageFolder folder)
    {
        const auto* source = FindImageSource(id);
        if (!source)
            throw winrt::hresult_invalid_argument(L"OnyxOS supports only the Windows 11 installer.");

        // Microsoft requires edition/language selection and provides a
        // time-limited download URL. Never save the download page as an ISO.
        if (source->kind != ImageSourceKind::DirectIso || source->url.empty())
            throw winrt::hresult_error(E_NOTIMPL,
                L"Open Microsoft's official Windows 11 download page, select the ISO options, and save the ISO into LocalState\\Windows11\\ISO.");

        auto file = co_await folder.CreateFileAsync(
            winrt::hstring(source->fileName), ws::CreationCollisionOption::ReplaceExisting);
        wnbt::BackgroundDownloader downloader;
        auto operation = downloader.CreateDownload(wf::Uri(winrt::hstring(source->url)), file);
        co_await operation.StartAsync();

        const auto properties = co_await file.GetBasicPropertiesAsync();
        if (properties.Size() < 65536)
        {
            co_await file.DeleteAsync();
            throw winrt::hresult_error(E_FAIL, L"Downloaded file is too small to be a valid Windows 11 ISO.");
        }
        co_return file;
    }
}
