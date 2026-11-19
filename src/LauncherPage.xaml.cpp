#include "LauncherPage.xaml.h"
#if __has_include("LauncherPage.g.cpp")
#include "LauncherPage.g.cpp"
#endif

namespace winrt::OnyxOS::implementation
{
    LauncherPage::LauncherPage()
    {
        InitializeComponent();
    }

    void LauncherPage::ProfileCard_Click(
        Windows::Foundation::IInspectable const& sender,
        Windows::UI::Xaml::RoutedEventArgs const&)
    {
        auto card = sender.try_as<Windows::UI::Xaml::Controls::Button>();
        if (!card)
        {
            return;
        }

        auto tag = winrt::unbox_value_or<winrt::hstring>(card.Tag(), L"");
        m_selectedProfile = tag;

        if (tag == L"windows10") m_selectedDisplayName = L"Windows 10";
        else if (tag == L"windows11") m_selectedDisplayName = L"Windows 11";
        else if (tag == L"debian") m_selectedDisplayName = L"Debian";
        else if (tag == L"ubuntu") m_selectedDisplayName = L"Ubuntu";
        else if (tag == L"archlinux") m_selectedDisplayName = L"Arch Linux";
        else if (tag == L"macos") m_selectedDisplayName = L"macOS (experimental)";
        else m_selectedDisplayName = L"Perfil desconhecido";

        SelectionTitle().Text(L"Selecionado: " + m_selectedDisplayName);
        StatusText().Text(L"Perfil selecionado. O mecanismo QEMU ainda precisa ser integrado.");
        LaunchButton().IsEnabled(false);
    }

    void LauncherPage::LaunchButton_Click(
        Windows::Foundation::IInspectable const&,
        Windows::UI::Xaml::RoutedEventArgs const&)
    {
        // Intentionally unavailable until the QEMU host, VM configuration,
        // storage picker, and lifecycle API are integrated and validated.
        StatusText().Text(L"A inicialização de máquinas virtuais ainda não está implementada.");
        LaunchButton().IsEnabled(false);
    }
}
