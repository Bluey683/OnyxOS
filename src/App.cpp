#include <windows.h>
#include <cstdint>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Media.h>

namespace
{
    using namespace winrt;
    using namespace Windows::UI;
    using namespace Windows::UI::Xaml;
    using namespace Windows::UI::Xaml::Controls;
    using namespace Windows::UI::Xaml::Media;

    SolidColorBrush MakeBrush(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
    {
        Color color{};
        color.A = 255;
        color.R = red;
        color.G = green;
        color.B = blue;
        return SolidColorBrush(color);
    }

    TextBlock MakeText(
        winrt::hstring const& value,
        double size,
        Windows::UI::Xaml::Media::Brush const& foreground)
    {
        TextBlock text;
        text.Text(value);
        text.FontSize(size);
        text.Foreground(foreground);
        text.TextWrapping(TextWrapping::Wrap);
        return text;
    }
}

struct OnyxOSApp :
    winrt::implements<OnyxOSApp, winrt::Windows::UI::Xaml::ApplicationT<OnyxOSApp>>
{
    void OnLaunched(
        winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&)
    {
        using namespace Windows::UI::Xaml;
        using namespace Windows::UI::Xaml::Controls;

        auto window = Window::Current();

        auto page = StackPanel();
        page.Spacing(14);
        page.Padding(Thickness{ 28, 24, 28, 24 });
        page.Background(MakeBrush(12, 19, 30));

        auto title = MakeText(L"ONYXOS", 32, MakeBrush(244, 247, 251));
        title.FontWeight(Windows::UI::Text::FontWeights::Bold());
        page.Children().Append(title);

        page.Children().Append(
            MakeText(L"VIRTUAL MACHINE CENTER", 12, MakeBrush(139, 154, 175)));

        auto heading = MakeText(
            L"Escolha seu sistema", 25, MakeBrush(244, 247, 251));
        heading.Margin(Thickness{ 0, 18, 0, 0 });
        page.Children().Append(heading);

        page.Children().Append(MakeText(
            L"Selecione um perfil para preparar sua máquina virtual.",
            14, MakeBrush(154, 169, 188)));

        auto selection = MakeText(
            L"Nenhum sistema selecionado", 16, MakeBrush(244, 247, 251));
        selection.Margin(Thickness{ 0, 14, 0, 0 });
        page.Children().Append(selection);

        auto status = MakeText(
            L"Launcher iniciado. O mecanismo de inicialização das VMs ainda precisa ser integrado.",
            13, MakeBrush(154, 169, 188));
        page.Children().Append(status);

        auto launch = Button();
        launch.Content(box_value(L"Iniciar máquina virtual"));
        launch.IsEnabled(false);
        launch.MinHeight(52);
        launch.Margin(Thickness{ 0, 8, 0, 0 });
        launch.Background(MakeBrush(32, 49, 70));
        launch.Foreground(MakeBrush(244, 247, 251));
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
            card.Background(MakeBrush(20, 32, 49));
            card.Foreground(MakeBrush(244, 247, 251));
            card.BorderBrush(MakeBrush(41, 59, 80));
            card.BorderThickness(Thickness{ 1 });
            card.Click([selection, status](IInspectable const& sender, RoutedEventArgs const&)
            {
                auto selected = sender.as<Button>();
                auto id = unbox_value<winrt::hstring>(selected.Tag());
                auto name = unbox_value<winrt::hstring>(selected.Content());
                selection.Text(winrt::hstring(L"Selecionado: ") + name);
                status.Text(winrt::hstring(L"Perfil ") + id +
                    L" selecionado. O mecanismo QEMU ainda precisa ser integrado.");
            });
            page.Children().Append(card);
        }

        page.Children().Append(MakeText(
            L"ONYXOS  |  QEMU HOST  |  ISO / VHDX / CONFIGURACOES",
            11, MakeBrush(101, 117, 138)));

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
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        winrt::Windows::UI::Xaml::Application::Start([](auto&&)
        {
            winrt::make<OnyxOSApp>();
        });
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}
