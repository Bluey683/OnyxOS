#include "App.xaml.h"
#include "OnyxLogger.h"
#include "vm/VirtualMachine.h"

using namespace Platform;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace OnyxOS
{
    App::App()
    {
        // Keep App.xaml out of the startup path: Xbox confirmed that its
        // InitializeComponent call fails before the application can launch.
        OnyxLogger::App(L"App constructor entered; App.xaml loading bypassed.");
        UnhandledException += ref new UnhandledExceptionEventHandler(
            this, &App::OnUnhandledException);
        OnyxLogger::App(L"Unhandled-exception logger registered.");
    }

    void App::OnUnhandledException(Object^, UnhandledExceptionEventArgs^ args)
    {
        OnyxLogger::App(L"UNHANDLED EXCEPTION: " + args->Message);
    }

    void App::OnLaunched(
        Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^)
    {
        OnyxLogger::App(L"OnLaunched entered; building native C++ launcher UI.");
        try
        {
            auto window = Window::Current;
            auto root = ref new Grid();
            root->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 11, 16, 24));
            root->Padding = Thickness(32);

            auto layout = ref new StackPanel();
            layout->Spacing = 18;

            auto heading = ref new TextBlock();
            heading->Text = L"ONYXOS";
            heading->FontSize = 34;
            heading->FontWeight = Windows::UI::Text::FontWeights::Bold;
            heading->Foreground = ref new SolidColorBrush(Colors::White);
            layout->Children->Append(heading);

            auto subtitle = ref new TextBlock();
            subtitle->Text = L"VIRTUAL MACHINE CENTER";
            subtitle->FontSize = 13;
            subtitle->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 139, 154, 175));
            layout->Children->Append(subtitle);

            auto prompt = ref new TextBlock();
            prompt->Text = L"Escolha seu sistema";
            prompt->FontSize = 25;
            prompt->Margin = Thickness(0, 18, 0, 0);
            prompt->Foreground = ref new SolidColorBrush(Colors::White);
            layout->Children->Append(prompt);

            auto hint = ref new TextBlock();
            hint->Text = L"Selecione um perfil para preparar sua m\u00E1quina virtual.";
            hint->FontSize = 15;
            hint->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 154, 169, 188));
            layout->Children->Append(hint);

            auto status = ref new TextBlock();
            status->Text = L"Nenhum sistema selecionado.";
            status->FontSize = 17;
            status->TextWrapping = TextWrapping::Wrap;
            status->Margin = Thickness(0, 12, 0, 0);
            status->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 229, 184, 95));
            layout->Children->Append(status);

            auto profiles = ref new StackPanel();
            profiles->Spacing = 8;
            profiles->Margin = Thickness(0, 8, 0, 0);

            const wchar_t* names[] = {
                L"Windows 10", L"Windows 11", L"Debian",
                L"Ubuntu", L"Arch Linux", L"macOS (experimental)"
            };
            const char* profileIds[] = {
                "windows10", "windows11", "debian", "ubuntu", "arch", "macos"
            };

            for (int i = 0; i < 6; ++i)
            {
                String^ name = ref new String(names[i]);
                std::string profileId(profileIds[i]);
                auto button = ref new Button();
                button->Content = name;
                button->Tag = name;
                button->HorizontalAlignment = HorizontalAlignment::Stretch;
                button->HorizontalContentAlignment = HorizontalAlignment::Left;
                button->MinHeight = 58;
                button->Padding = Thickness(18, 10, 18, 10);
                button->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 20, 32, 49));
                button->Foreground = ref new SolidColorBrush(Colors::White);
                button->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 41, 59, 80));
                button->BorderThickness = Thickness(1);

                button->Click += ref new RoutedEventHandler(
                    [status, name, profileId](Object^, RoutedEventArgs^)
                    {
                        auto config = onyx::vm::CreateProfile(profileId);
                        if (!config.enabled)
                        {
                            status->Text = L"Selecionado: " + name + L"\nPerfil experimental indisponível para inicialização.";
                            OnyxLogger::App(L"Disabled experimental profile selected: " + name);
                            return;
                        }
                        std::wstring details = L"Selecionado: " + std::wstring(name->Data()) +
                            L"\nV\u00EDdeo: " + std::to_wstring(config.displayWidth) + L"x" + std::to_wstring(config.displayHeight) +
                            L" @ " + std::to_wstring(config.displayRefreshHz) + L" Hz" +
                            L"\nMem\u00F3ria: " + std::to_wstring(config.memoryMB) + L" MB | CPU: " + std::to_wstring(config.cpuCount) +
                            L"\nAguardando ISO e disco virtual. QEMU/D3D ainda n\u00E3o iniciados.";
                        status->Text = ref new String(details.c_str());
                        OnyxLogger::App(L"OS profile selected; VM/display defaults loaded: " + name);
                    });
                profiles->Children->Append(button);
            }

            auto scroll = ref new ScrollViewer();
            scroll->VerticalScrollBarVisibility = ScrollBarVisibility::Auto;
            scroll->Content = profiles;
            layout->Children->Append(scroll);

            auto footer = ref new TextBlock();
            footer->Text = L"ONYXOS  |  QEMU HOST  |  ISO / VHDX / CONFIGURACOES";
            footer->FontSize = 11;
            footer->Margin = Thickness(0, 12, 0, 0);
            footer->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 101, 117, 138));
            layout->Children->Append(footer);

            root->Children->Append(layout);
            window->Content = root;
            window->Activate();
            OnyxLogger::App(L"Native launcher UI activated successfully; XAML pages are bypassed.");
        }
        catch (Exception^ ex)
        {
            OnyxLogger::App(L"NATIVE LAUNCHER UI FAILURE: " + ex->Message);
            throw;
        }
    }
}

[MTAThread]
int main(Array<String^>^)
{
    OutputDebugStringW(L"[OnyxOS][APP] main() entered.\n");
    try
    {
        OutputDebugStringW(L"[OnyxOS][APP] Entering Application::Start.\n");
        Application::Start(ref new ApplicationInitializationCallback(
            [](ApplicationInitializationCallbackParams^)
            {
                OnyxOS::OnyxLogger::App(L"Application initialization callback entered.");
                auto app = ref new OnyxOS::App();
            }));
        OnyxOS::OnyxLogger::App(L"Application::Start returned normally.");
    }
    catch (Exception^ ex)
    {
        OnyxOS::OnyxLogger::App(L"FATAL PLATFORM EXCEPTION IN main: " + ex->Message);
        return -1;
    }
    catch (const std::exception& ex)
    {
        OnyxOS::OnyxLogger::App(L"FATAL C++ EXCEPTION IN main.");
        OutputDebugStringA(ex.what());
        OutputDebugStringA("\n");
        return -2;
    }
    catch (...)
    {
        OnyxOS::OnyxLogger::App(L"FATAL UNKNOWN EXCEPTION IN main.");
        return -3;
    }
    return 0;
}
