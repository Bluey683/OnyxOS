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

    void InstallerPage::ShowControlsButton_Click(Object^, RoutedEventArgs^)
    {
        HeaderPanel->Visibility = Windows::UI::Xaml::Visibility::Visible;
        FooterPanel->Visibility = Windows::UI::Xaml::Visibility::Visible;
        ShowControlsButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
}
