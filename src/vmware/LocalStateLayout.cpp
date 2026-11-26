#include "LocalStateLayout.h"

#include <winrt/Windows.Storage.h>

#include <array>
#include <string_view>

namespace VMwareXbox::Storage
{
    winrt::Windows::Foundation::IAsyncAction InitializeLocalStateAsync()
    {
        auto local = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
        auto root = co_await local.CreateFolderAsync(
            L"VMware",
            winrt::Windows::Storage::CreationCollisionOption::OpenIfExists);

        constexpr std::array<std::wstring_view, 7> folders{
            L"Config",
            L"Virtual Machines",
            L"ISO",
            L"Disks",
            L"Snapshots",
            L"Logs",
            L"Cache"
        };

        for (auto const& name : folders)
        {
            co_await root.CreateFolderAsync(
                winrt::hstring(name.data(), static_cast<winrt::hstring::size_type>(name.size())),
                winrt::Windows::Storage::CreationCollisionOption::OpenIfExists);
        }
    }
}
