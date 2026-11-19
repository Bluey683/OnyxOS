#include <windows.h>
#include <collection.h>
#include <ppltasks.h>

using namespace Platform;
using namespace Windows::UI;
using namespace Windows::UI::Text;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace
{
    SolidColorBrush^ MakeBrush(BYTE red, BYTE green, BYTE blue)
    {
        Color color;
        color.A = 255;
        color.R = red;
        color.G = green;
        color.B = blue;
        return ref new SolidColorBrush(color);
    }

    TextBlock^ MakeText(String^ value, double size, Brush^ foreground)
    {
        auto text = ref new TextBlock();
        text->Text = value;
        text->FontSize = size;
        text->Foreground = foreground;
        text->TextWrapping = TextWrapping::Wrap;
        return text;
    }
}

ref class OnyxApp sealed : public Application
{
public:
    OnyxApp()
    {
        selection = nullptr;
        status = nullptr;
    }

protected:
    virtual void OnLaunched(LaunchActivatedEventArgs^ args) override
    {
        auto window = Window::Current;

        auto page = ref new StackPanel();
        page->Spacing = 14;
        page->Padding = Thickness(28, 24, 28, 24);
        page->Background = MakeBrush(12, 19, 30);

        auto title = MakeText(ref new String(L"ONYXOS"), 32, MakeBrush(244, 247, 251));
        title->FontWeight = FontWeights::Bold;
        page->Children->Append(title);

        page->Children->Append(
            MakeText(ref new String(L"VIRTUAL MACHINE CENTER"), 12, MakeBrush(139, 154, 175)));

        auto heading = MakeText(
            ref new String(L"Escolha seu sistema"), 25, MakeBrush(244, 247, 251));
        heading->Margin = Thickness(0, 18, 0, 0);
        page->Children->Append(heading);

        page->Children->Append(MakeText(
            ref new String(L"Selecione um perfil para preparar sua máquina virtual."),
            14, MakeBrush(154, 169, 188)));

        selection = MakeText(
            ref new String(L"Nenhum sistema selecionado"), 16, MakeBrush(244, 247, 251));
        selection->Margin = Thickness(0, 14, 0, 0);
        page->Children->Append(selection);

        status = MakeText(
            ref new String(L"Launcher iniciado. O mecanismo de inicializacao das VMs ainda precisa ser integrado."),
            13, MakeBrush(154, 169, 188));
        page->Children->Append(status);

        auto launch = ref new Button();
        launch->Content = ref new String(L"Iniciar maquina virtual");
        launch->IsEnabled = false;
        launch->MinHeight = 52;
        launch->Margin = Thickness(0, 8, 0, 0);
        launch->Background = MakeBrush(32, 49, 70);
        launch->Foreground = MakeBrush(244, 247, 251);
        page->Children->Append(launch);

        struct Profile
        {
            wchar_t const* id;
            wchar_t const* name;
        };

        static const Profile profiles[] = {
            { L"windows10", L"Windows 10" },
            { L"windows11", L"Windows 11" },
            { L"debian", L"Debian" },
            { L"ubuntu", L"Ubuntu" },
            { L"archlinux", L"Arch Linux" },
            { L"macos", L"macOS (experimental)" }
        };

        for (auto const& profile : profiles)
        {
            auto card = ref new Button();
            card->Content = ref new String(profile.name);
            card->Tag = ref new String(profile.id);
            card->MinHeight = 64;
            card->HorizontalContentAlignment = HorizontalAlignment::Left;
            card->Padding = Thickness(18, 12, 18, 12);
            card->Background = MakeBrush(20, 32, 49);
            card->Foreground = MakeBrush(244, 247, 251);
            card->BorderBrush = MakeBrush(41, 59, 80);
            card->BorderThickness = Thickness(1);
            card->Click += ref new RoutedEventHandler(this, &OnyxApp::OnProfileClick);
            page->Children->Append(card);
        }

        page->Children->Append(MakeText(
            ref new String(L"ONYXOS  |  QEMU HOST  |  ISO / VHDX / CONFIGURACOES"),
            11, MakeBrush(101, 117, 138)));

        auto scroll = ref new ScrollViewer();
        scroll->VerticalScrollBarVisibility = ScrollBarVisibility::Auto;
        scroll->Content = page;

        window->Content = scroll;
        window->Activate();
    }

private:
    TextBlock^ selection;
    TextBlock^ status;

    void OnProfileClick(Object^ sender, RoutedEventArgs^)
    {
        auto selected = safe_cast<Button^>(sender);
        auto id = safe_cast<String^>(selected->Tag);
        auto name = safe_cast<String^>(selected->Content);

        selection->Text = String::Concat(ref new String(L"Selecionado: "), name);
        status->Text = String::Concat(
            String::Concat(ref new String(L"Perfil "), id),
            ref new String(L" selecionado. O mecanismo QEMU ainda precisa ser integrado."));
    }
};

[MTAThread]
int main(Array<String^>^)
{
    Application::Start(ref new ApplicationInitializationCallback(
        [](ApplicationInitializationCallbackParams^)
        {
            auto app = ref new OnyxApp();
        }));
    return 0;
}
