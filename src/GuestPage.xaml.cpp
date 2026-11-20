#include "GuestPage.xaml.h"

using namespace Platform;
using namespace Windows::UI::Xaml;

namespace OnyxOS
{
    GuestPage::GuestPage()
    {
        InitializeComponent();
    }

    void GuestPage::SetGuestProfile(String^ id, String^ name)
    {
        GuestTitle->Text = L"ONYXOS • " + name;
        GuestSubtitle->Text = L"QEMU x86_64 • TCG software emulation";
        ConnectionStatus->Text = L"Perfil: " + id + L" • aguardando QEMU";
    }

    void GuestPage::SetDisplayStatus(String^ status, bool hasFrame)
    {
        DisplayStatus->Text = status;
        NoFramePanel->Visibility = hasFrame
            ? Windows::UI::Xaml::Visibility::Collapsed
            : Windows::UI::Xaml::Visibility::Visible;
    }

    void GuestPage::SetRendererName(String^ renderer)
    {
        RendererLabel->Text = renderer;
    }

    Windows::UI::Xaml::Controls::SwapChainPanel^ GuestPage::GetQemuSurface()
    {
        return QemuSurface;
    }

    Windows::UI::Xaml::Controls::TextBox^ GuestPage::GetKeyboardSink()
    {
        return KeyboardSink;
    }

    void GuestPage::SetStopEnabled(bool enabled)
    {
        StopButton->IsEnabled = enabled;
    }

    void GuestPage::StopButton_Click(Object^, RoutedEventArgs^)
    {
        StopButton->IsEnabled = false;
        StopRequested(this, nullptr);
    }

    void GuestPage::EnterFullScreen()
    {
        HeaderPanel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        FooterPanel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        ShowControlsButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    void GuestPage::FullScreenButton_Click(Object^, RoutedEventArgs^)
    {
        EnterFullScreen();
    }

    void GuestPage::ShowControlsButton_Click(Object^, RoutedEventArgs^)
    {
        HeaderPanel->Visibility = Windows::UI::Xaml::Visibility::Visible;
        FooterPanel->Visibility = Windows::UI::Xaml::Visibility::Visible;
        ShowControlsButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
}
