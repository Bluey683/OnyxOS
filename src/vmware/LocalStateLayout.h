#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>

namespace VMwareXbox::Storage
{
    // Creates app-private folders under ApplicationData::Current().LocalFolder.
    // Call from the packaged app; never writes to the user's Documents/Desktop folders.
    winrt::Windows::Foundation::IAsyncAction InitializeLocalStateAsync();

    winrt::Windows::Storage::StorageFolder GetVmwareRoot();
}
