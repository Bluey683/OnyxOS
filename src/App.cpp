#include <windows.h>
#include <cstdint>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace
{
    SolidColorBrush Brush(std::uint8_t r, std::uint8_t g, std::uint8_t b)
    {
        return SolidColorBrush(Color{ 255, r, g, b });
    }

    TextBlock MakeText(hstring const& value, double size, SolidColorBrush const& color)
    {
        TextBlock text;
        text.Text(value);
        text.FontSize(size);
        text.Foreground(color);
        text.TextWrapping(TextWrapping::Wrap);
        return text;
    }
}

struct OnyxOSApp : implements<OnyxOSApp, ApplicationT<OnyxOSApp>>
{
    void OnLaunched(LaunchActivatedEventArgs const&)
    {
        auto window = Window::Current();

        auto page = StackPanel();
        page.Spacing(16);
        page.Padding(Thickness{ 28, 24, 28, 24 });

        auto title = MakeText(L"ONYXOS", 32, Brush(244, 247, 251));
        title.FontWeight(Windows::UI::Text::FontWeights::Bold());
        page.Children().Append(title);

        auto subtitle = MakeText(L"VIRTUAL MACHINE CENTER", 12, Brush(139, 154, 175));
        page.Children().Append(subtitle);

        auto heading = MakeText(L"Escolha seu sistema", 25, Brush(244, 247, 251));
        heading.Margin(Thickness{ 0, 18, 0, 0 });
        page.Children().Append(heading);

        auto description = MakeText(
            L"Selecione um perfil para preparar sua máquina virtual.",
            14, Brush(154, 169, 188));
        page.Children().Append(description);

        auto selection = MakeText(L"Nenhum sistema selecionado", 16, Brush(244, 247, 251));
        selection.Margin(Thickness{ 0, 14, 0, 0 });
        page.Children().Append(selection);

        auto status = MakeText(
            L"Launcher iniciado. A inicialização das máquinas virtuais ainda não está disponível.",
            13, Brush(154, 169, 188));
        page.Children().Append(status);

        auto launch = Button();
        launch.Content(box_value(L"Iniciar máquina virtual"));
        launch.IsEnabled(false);
        launch.MinHeight(52);
        launch.Margin(Thickness{ 0, 8, 0, 0 });
        launch.Background(Brush(32, 49, 70));
        launch.Foreground(Brush(244, 247, 251));
        page.Children().Append(launch);

        struct Profile
        {
            wchar_t const* id;
            wchar_t const* name;
        };

        constexpr Profile profiles[] = {
            { L"windows10", L"Windows 10" },
            { L"windows11", L"Windows 11" },
            { L"debian", L"Debian" },
            { L"ubuntu", L"Ubuntu" },
            { L"archlinux", L"Arch Linux" },
            { L"macos", L"macOS (experimental)" }
        };

        for (auto const& profile : profiles)
        {
            auto card = Button();
            card.Content(box_value(profile.name));
            card.Tag(box_value(profile.id));
            card.MinHeight(64);
            card.HorizontalContentAlignment(HorizontalAlignment::Left);
            card.Padding(Thickness{ 18, 12, 18, 12 });
            card.Background(Brush(20, 32, 49));
            card.Foreground(Brush(244, 247, 251));
            card.BorderBrush(Brush(41, 59, 80));
            card.BorderThickness(Thickness{ 1 });
            card.Click([selection, status, launch](IInspectable const& sender, RoutedEventArgs const&)
            {
                auto selected = sender.as<Button>();
                auto id = unbox_value<hstring>(selected.Tag());
                auto name = unbox_value<hstring>(selected.Content());
                selection.Text(L"Selecionado: " + name);
                status.Text(L"Perfil " + id + L" selecionado. O mecanismo QEMU ainda precisa ser integrado.");
                launch.IsEnabled(false);
            });
            page.Children().Append(card);
        }

        auto footer = MakeText(
            L"ONYXOS  •  QEMU HOST  •  ISO / VHDX / CONFIGURAÇÕES",
            11, Brush(101, 117, 138));
        footer.Margin(Thickness{ 0, 16, 0, 0 });
        page.Children().Append(footer);

        auto scroll = ScrollViewer();
        scroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);
        scroll.Content(page);

        window.Content(scroll);
        window.Activate();
    }
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    try
    {
        Application::Start([](auto&&)
        {
            make<OnyxOSApp>();
        });
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}
