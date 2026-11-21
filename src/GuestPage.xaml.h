#pragma once

#include "src/GuestPage.g.h"

namespace OnyxOS
{
    public ref class GuestPage sealed
    {
    public:
        GuestPage();
        void SetGuestProfile(Platform::String^ profileId, Platform::String^ displayName);
        void SetDisplayStatus(Platform::String^ status, bool hasFrame);
        void SetRendererName(Platform::String^ renderer);
        Windows::UI::Xaml::Controls::SwapChainPanel^ GetQemuSurface();
        Windows::UI::Xaml::Controls::TextBox^ GetKeyboardSink();
        void SetStopEnabled(bool enabled);
        void EnterFullScreen();

        event Windows::Foundation::EventHandler<Platform::Object^>^ StopRequested;
        event Windows::Foundation::EventHandler<Platform::String^>^ VirtualKeyRequested;
        event Windows::Foundation::EventHandler<Platform::String^>^ VirtualKeyboardModeChanged;

    private:
        void StopButton_Click(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void FullScreenButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void ShowControlsButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void KeyboardToggleButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
        void VirtualKeyButton_Click(Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
    };
}
