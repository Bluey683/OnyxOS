#include "LocalStateLayout.h"

#include <array>
#include <string_view>

using namespace winrt;
using namespace Windows::Storage;

namespace VMwareXbox::Storage
{
    StorageFolder GetVmwareRoot()
    {
        return ApplicationData::Current().LocalFolder()
            .CreateFolderAsync(L"VMware", CreationCollisionOption::OpenIfExists).get();
    }

    IAsyncAction InitializeLocalStateAsync()
    {
        auto local = ApplicationData::Current().LocalFolder();
        auto root = co_await local.CreateFolderAsync(
            L"VMware", CreationCollisionOption::OpenIfExists);

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
                hstring(name), CreationCollisionOption::OpenIfExists);
        }
    }
}
