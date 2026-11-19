#pragma once

#include "src/MainPage.g.h"

namespace OnyxOS
{
    public ref class MainPage sealed
    {
    public:
        MainPage();

    private:
        void ProfileCard_Click(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void LaunchButton_Click(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
    };
}
