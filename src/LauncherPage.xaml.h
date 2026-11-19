#pragma once

#include "LauncherPage.g.h"

namespace winrt::OnyxOS::implementation
{
    struct LauncherPage : LauncherPageT<LauncherPage>
    {
        LauncherPage();

        void ProfileCard_Click(
            Windows::Foundation::IInspectable const& sender,
            Windows::UI::Xaml::RoutedEventArgs const& args);

        void LaunchButton_Click(
            Windows::Foundation::IInspectable const& sender,
            Windows::UI::Xaml::RoutedEventArgs const& args);

    private:
        winrt::hstring m_selectedProfile;
        winrt::hstring m_selectedDisplayName;
    };
}

namespace winrt::OnyxOS::factory_implementation
{
    struct LauncherPage : LauncherPageT<LauncherPage, implementation::LauncherPage>
    {
    };
}
