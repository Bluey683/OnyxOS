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
            ? Visibility::Collapsed
            : Visibility::Visible;
    }

    void GuestPage::SetRendererName(String^ renderer)
    {
        RendererLabel->Text = renderer;
    }

    void GuestPage::StopButton_Click(Object^, RoutedEventArgs^)
    {
        DisplayStatus->Text = L"O controle de parada ainda não está conectado ao ciclo de vida do QEMU.";
    }
}
