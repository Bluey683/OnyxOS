#include "InstallerPage.xaml.h"

using namespace Platform;
using namespace Windows::UI::Xaml;

namespace OnyxOS
{
    InstallerPage::InstallerPage()
    {
        InitializeComponent();
    }

    Windows::UI::Xaml::Controls::Grid^ InstallerPage::GetGuestHost()
    {
        return GuestHost;
    }

    Windows::UI::Xaml::Controls::TextBox^ InstallerPage::GetKeyboardSink()
    {
        return KeyboardSink;
    }

    void InstallerPage::SetStatus(String^ status)
    {
        InstallStatus->Text = status;
    }

    void InstallerPage::SetStopped()
    {
        InstallProgress->IsIndeterminate = false;
        InstallProgress->Value = 100;
        InstallStatus->Text = L"QEMU foi parado. Se o instalador terminou e o sistema desligou corretamente, confirme para registrar a instalação.";
        StopInstallerButton->IsEnabled = false;
        FinishInstallerButton->IsEnabled = true;
    }

    void InstallerPage::StopInstallerButton_Click(Object^, RoutedEventArgs^)
    {
        StopInstallerButton->IsEnabled = false;
        InstallStatus->Text = L"Solicitando parada do QEMU...";
        StopRequested(this, nullptr);
    }

    void InstallerPage::FinishInstallerButton_Click(Object^, RoutedEventArgs^)
    {
        FinishInstallerButton->IsEnabled = false;
        FinishRequested(this, nullptr);
    }

    void InstallerPage::EnterFullScreen()
    {
        HeaderPanel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        FooterPanel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        ShowControlsButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    void InstallerPage::FullScreenButton_Click(Object^, RoutedEventArgs^)
    {
        EnterFullScreen();
    }

    void InstallerPage::KeyboardToggleButton_Click(Object^, RoutedEventArgs^)
    {
        const bool show = VirtualKeyboardPanel->Visibility != Windows::UI::Xaml::Visibility::Visible;
        VirtualKeyboardPanel->Visibility = show ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
        KeyboardToggleButton->Content = show ? L"Fechar teclado" : L"Teclado";
        VirtualKeyboardModeChanged(this, show ? L"1" : L"0");
        if (show) Key1Button->Focus(Windows::UI::Xaml::FocusState::Programmatic);
        else KeyboardSink->Focus(Windows::UI::Xaml::FocusState::Programmatic);
    }

    void InstallerPage::VirtualKeyButton_Click(Object^ sender, RoutedEventArgs^)
    {
        auto button = dynamic_cast<Windows::UI::Xaml::Controls::Button^>(sender);
        if (button == nullptr || button->Tag == nullptr || !VirtualKeyRequested) return;
        auto key = dynamic_cast<String^>(button->Tag);
        if (key != nullptr) VirtualKeyRequested(this, key);
    }

    void InstallerPage::ShowControlsButton_Click(Object^, RoutedEventArgs^)
    {
        const bool controlsVisible = HeaderPanel->Visibility == Windows::UI::Xaml::Visibility::Visible;
        HeaderPanel->Visibility = controlsVisible ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
        FooterPanel->Visibility = controlsVisible ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
        ShowControlsButton->Content = controlsVisible ? L"Mostrar controles" : L"Ocultar controles";
    }
}
