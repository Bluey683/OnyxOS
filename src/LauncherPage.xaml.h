#pragma once

#include "src/LauncherPage.g.h"

namespace OnyxOS
{
    public ref class LauncherPage sealed : public Windows::UI::Xaml::Controls::Page
    {
    public:
        LauncherPage();

    private:
        void ProfileCard_Click(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);

        void LaunchButton_Click(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
    };
}
