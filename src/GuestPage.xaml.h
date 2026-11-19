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

    private:
        void StopButton_Click(
            Platform::Object^ sender,
            Windows::UI::Xaml::RoutedEventArgs^ args);
    };
}
