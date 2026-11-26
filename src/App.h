#pragma once

#include "App.g.h"

namespace winrt::OnyxOS::implementation
{
    struct App : AppT<App>
    {
        App() = default;
        Windows::ApplicationModel::Core::IFrameworkView CreateView();
    };
}
