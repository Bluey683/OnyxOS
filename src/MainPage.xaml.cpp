#include "MainPage.xaml.h"
#include "OnyxLogger.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace OnyxOS
{
    MainPage::MainPage()
    {
        InitializeComponent();
        OnyxLogger::App(L"MainPage constructed.");
    }

    void MainPage::ProfileCard_Click(Object^ sender, RoutedEventArgs^)
    {
        auto card = dynamic_cast<Button^>(sender);
        if (card == nullptr)
            return;

        auto tag = dynamic_cast<String^>(card->Tag);
        String^ displayName = L"Perfil desconhecido";

        if (tag == L"windows10") displayName = L"Windows 10";
        else if (tag == L"windows11") displayName = L"Windows 11";
        else if (tag == L"debian") displayName = L"Debian";
        else if (tag == L"ubuntu") displayName = L"Ubuntu";
        else if (tag == L"archlinux") displayName = L"Arch Linux";
        else if (tag == L"macos") displayName = L"macOS (experimental)";

        OnyxLogger::App(L"Profile selected: " + displayName);
        SelectionTitle->Text = L"Selecionado: " + displayName;
        StatusText->Text = L"Perfil selecionado. O mecanismo QEMU ainda precisa ser integrado.";
        LaunchButton->IsEnabled = false;
    }

    void MainPage::LaunchButton_Click(Object^, RoutedEventArgs^)
    {
        OnyxLogger::Qemu(L"Launch requested, but VM startup is not connected yet.");
        StatusText->Text = L"A inicialização de máquinas virtuais ainda não está implementada.";
        LaunchButton->IsEnabled = false;
    }
}
