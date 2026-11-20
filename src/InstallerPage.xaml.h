#pragma once
#include "src/InstallerPage.g.h"

namespace OnyxOS
{
    public ref class InstallerPage sealed
    {
    public:
        InstallerPage();
        Windows::UI::Xaml::Controls::Grid^ GetGuestHost();
        void SetStatus(Platform::String^ status);
        void SetStopped();

        event Windows::Foundation::EventHandler<Platform::Object^>^ StopRequested;
        event Windows::Foundation::EventHandler<Platform::Object^>^ FinishRequested;

    private:
        void StopInstallerButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void FinishInstallerButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void FullScreenButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void ShowControlsButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
    };
}
