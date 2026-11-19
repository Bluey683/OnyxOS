#include "OsImageDownloader.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.h>
#include <winrt/Windows.Networking.BackgroundTransfer.h>
#include <winrt/Windows.Storage.h>
#include <array>
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Networking;
using namespace Windows::Networking::BackgroundTransfer;
using namespace Windows::Storage;
namespace onyx::downloads {
namespace {
const std::array<OsImageSource,5> Sources{{
{"debian",L"debian.iso",L"https://cloudfront.debian.net/debian-cd/current/amd64/iso-cd/debian-13.7.0-amd64-netinst.iso",L"https://www.debian.org/download",ImageSourceKind::DirectIso},
{"ubuntu",L"ubuntu.iso",L"https://releases.ubuntu.com/26.04/ubuntu-26.04.1-desktop-amd64.iso",L"https://ubuntu.com/download/desktop",ImageSourceKind::DirectIso},
{"arch-linux",L"archlinux.iso",L"https://geo.mirror.pkgbuild.com/iso/latest/archlinux-x86_64.iso",L"https://archlinux.org/download/",ImageSourceKind::DirectIso},
{"windows-10",L"windows-10.iso",L"",L"https://www.microsoft.com/software-download/windows10ISO",ImageSourceKind::MicrosoftDownloadPage},
{"windows-11",L"windows-11.iso",L"",L"https://www.microsoft.com/software-download/windows11",ImageSourceKind::MicrosoftDownloadPage}
}};
}
const OsImageSource* FindImageSource(const std::string& id) noexcept { for(const auto& s:Sources)if(s.profileId==id)return &s;return nullptr; }
IAsyncOperation<StorageFile> OsImageDownloader::DownloadOfficialIsoAsync(const std::string& id,StorageFolder folder) {
const auto* source=FindImageSource(id);
if(!source)throw hresult_invalid_argument(L"No official ISO source for this profile.");
if(source->kind!=ImageSourceKind::DirectIso||source->url.empty())
throw hresult_error(E_NOTIMPL,L"Microsoft requires edition/language selection on its official download page. Open the page and select the ISO.");
auto file=co_await folder.CreateFileAsync(source->fileName,CreationCollisionOption::ReplaceExisting);
BackgroundDownloader downloader;
auto op=downloader.CreateDownload(Uri(source->url),file);
co_await op.StartAsync();
auto props=co_await file.GetBasicPropertiesAsync();
if(props.Size()<65536){co_await file.DeleteAsync();throw hresult_error(E_FAIL,L"Downloaded file is too small to be a valid installer ISO.");}
co_return file;
}
}