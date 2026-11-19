#pragma once
#include "GuestPage.g.h"
namespace winrt::OnyxOS::implementation {
struct GuestPage : GuestPageT<GuestPage> {
    GuestPage();
    void SetGuestProfile(winrt::hstring const& profileId, winrt::hstring const& displayName);
    void SetDisplayStatus(winrt::hstring const& status, bool hasFrame);
    void SetRendererName(winrt::hstring const& renderer);
    void StopButton_Click(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&);
private:
    winrt::hstring m_profileId;
};
}
namespace winrt::OnyxOS::factory_implementation {
struct GuestPage : GuestPageT<GuestPage, implementation::GuestPage> {};
}
