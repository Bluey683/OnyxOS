#pragma once

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>

namespace winrt::OnyxOS::implementation
{
    struct App : winrt::implements<
        App,
        winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>
    {
        winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();
    };
}
