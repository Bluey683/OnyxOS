#pragma once

#include <winrt/Windows.Foundation.h>

namespace VMwareXbox::Storage
{
    // Creates app-private folders under ApplicationData::Current().LocalFolder.
    // Call during packaged-app startup; this never writes to user Documents/Desktop.
    winrt::Windows::Foundation::IAsyncAction InitializeLocalStateAsync();
}
