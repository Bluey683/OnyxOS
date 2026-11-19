#pragma once
#include <string>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
namespace onyx::downloads {
enum class ImageSourceKind { DirectIso, MicrosoftDownloadPage, Disabled };
struct OsImageSource { std::string profileId; std::wstring fileName; std::wstring url; std::wstring officialPage; ImageSourceKind kind{ImageSourceKind::Disabled}; };
const OsImageSource* FindImageSource(const std::string& profileId) noexcept;
class OsImageDownloader final {
public:
static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile>
DownloadOfficialIsoAsync(const std::string& profileId, winrt::Windows::Storage::StorageFolder destination);
};
}