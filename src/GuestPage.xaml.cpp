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

    void GuestPage::KeyboardToggleButton_Click(Object^, RoutedEventArgs^)
    {
        const bool show = VirtualKeyboardPanel->Visibility != Windows::UI::Xaml::Visibility::Visible;
        VirtualKeyboardPanel->Visibility = show ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
        KeyboardToggleButton->Content = show ? L"Fechar teclado" : L"Teclado";
        VirtualKeyboardModeChanged(this, show ? L"1" : L"0");
        if (show) Key1Button->Focus(Windows::UI::Xaml::FocusState::Programmatic);
        else KeyboardSink->Focus(Windows::UI::Xaml::FocusState::Programmatic);
    }

    void GuestPage::VirtualKeyButton_Click(Object^ sender, RoutedEventArgs^)
    {
        auto button = dynamic_cast<Windows::UI::Xaml::Controls::Button^>(sender);
        if (button == nullptr || button->Tag == nullptr) return;
        auto key = dynamic_cast<String^>(button->Tag);
        if (key != nullptr) VirtualKeyRequested(this, key);
    }

    void GuestPage::ShowControlsButton_Click(Object^, RoutedEventArgs^)
    {
        const bool controlsVisible = HeaderPanel->Visibility == Windows::UI::Xaml::Visibility::Visible;
        HeaderPanel->Visibility = controlsVisible ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
        FooterPanel->Visibility = controlsVisible ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
        ShowControlsButton->Content = controlsVisible ? L"Mostrar controles" : L"Ocultar controles";
    }
}
