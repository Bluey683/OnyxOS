#include "LauncherPage.xaml.h"
#include "LogsPage.xaml.h"
#include "OnyxLogger.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace OnyxOS
{
    LauncherPage::LauncherPage()
    {
        InitializeComponent();
        OnyxLogger::App(L"LauncherPage constructed.");
    }

    void LauncherPage::ProfileCard_Click(Object^ sender, RoutedEventArgs^)
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

    void LauncherPage::LaunchButton_Click(Object^, RoutedEventArgs^)
    {
        OnyxLogger::Qemu(L"Launch requested, but VM startup is not connected yet.");
        StatusText->Text = L"A inicialização de máquinas virtuais ainda não está implementada.";
        LaunchButton->IsEnabled = false;
    }

    void LauncherPage::LogsButton_Click(Object^, RoutedEventArgs^)
    {
        OnyxLogger::App(L"Opening in-app diagnostics.");
        auto frame = dynamic_cast<Frame^>(Window::Current->Content);
        if (frame != nullptr && !frame->Navigate(LogsPage::typeid))
            OnyxLogger::App(L"ERROR: navigation to LogsPage returned false.");
    }
}
