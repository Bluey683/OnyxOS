#include "GuestPage.xaml.h"
#if __has_include("GuestPage.g.cpp")
#include "GuestPage.g.cpp"
#endif
namespace winrt::OnyxOS::implementation {
GuestPage::GuestPage() { InitializeComponent(); }
void GuestPage::SetGuestProfile(winrt::hstring const& id, winrt::hstring const& name) {
    m_profileId=id;
    GuestTitle().Text(L"ONYXOS • "+name);
    GuestSubtitle().Text(L"QEMU x86_64 • TCG software emulation");
    ConnectionStatus().Text(L"Perfil: "+id+L" • aguardando QEMU");
}
void GuestPage::SetDisplayStatus(winrt::hstring const& status, bool hasFrame) {
    DisplayStatus().Text(status);
    NoFramePanel().Visibility(hasFrame ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible);
}
void GuestPage::SetRendererName(winrt::hstring const& renderer) { RendererLabel().Text(renderer); }
void GuestPage::StopButton_Click(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&) {
    // The QEMU lifecycle stop call will be wired when the guest session controller is connected.
    DisplayStatus().Text(L"O controle de parada ainda não está conectado ao ciclo de vida do QEMU.");
}
}
