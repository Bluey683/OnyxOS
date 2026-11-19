#include "App.xaml.h"
#include "OnyxLogger.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace OnyxOS
{
    App::App()
    {
        // Deliberately do not call InitializeComponent here. The Xbox log
        // proves that loading App.xaml currently throws E_FAIL before launch.
        // This diagnostic build starts the UWP Application without loading
        // any XAML resource dictionary so we can isolate the failing layer.
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
        OnyxLogger::App(L"OnLaunched entered; building diagnostic UI in C++.");
        try
        {
            auto window = Window::Current;
            auto root = ref new Grid();
            root->Background = ref new SolidColorBrush(Windows::UI::Colors::Black);

            auto stack = ref new StackPanel();
            stack->VerticalAlignment = VerticalAlignment::Center;
            stack->HorizontalAlignment = HorizontalAlignment::Center;
            stack->Spacing = 18;

            auto title = ref new TextBlock();
            title->Text = L"ONYXOS";
            title->Foreground = ref new SolidColorBrush(Windows::UI::Colors::White);
            title->FontSize = 42;
            title->FontWeight = Windows::UI::Text::FontWeights::Bold;
            title->HorizontalAlignment = HorizontalAlignment::Center;
            stack->Children->Append(title);

            auto status = ref new TextBlock();
            status->Text = L"Aplicativo iniciado.\nDiagnostico: XAML de inicializacao ignorado temporariamente.";
            status->Foreground = ref new SolidColorBrush(Windows::UI::Colors::LightGreen);
            status->FontSize = 20;
            status->TextAlignment = TextAlignment::Center;
            status->TextWrapping = TextWrapping::Wrap;
            status->MaxWidth = 900;
            stack->Children->Append(status);

            auto detail = ref new TextBlock();
            detail->Text = L"Esta tela confirma que o processo e a janela UWP permanecem ativos.\nA tela principal sera reativada depois de isolar o erro XAML.";
            detail->Foreground = ref new SolidColorBrush(Windows::UI::Colors::White);
            detail->FontSize = 16;
            detail->TextAlignment = TextAlignment::Center;
            detail->TextWrapping = TextWrapping::Wrap;
            detail->MaxWidth = 900;
            stack->Children->Append(detail);

            root->Children->Append(stack);
            window->Content = root;
            window->Activate();
            OnyxLogger::App(L"Diagnostic window activated successfully; MainPage navigation intentionally skipped.");
        }
        catch (Exception^ ex)
        {
            OnyxLogger::App(L"DIAGNOSTIC WINDOW FAILURE: " + ex->Message);
            throw;
        }
    }
}

[MTAThread]
int main(Array<String^>^)
{
    OutputDebugStringW(L"[OnyxOS][APP] main() entered.\\n");
    try
    {
        OutputDebugStringW(L"[OnyxOS][APP] Entering Application::Start.\\n");
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
        OutputDebugStringA("\\n");
        return -2;
    }
    catch (...)
    {
        OnyxOS::OnyxLogger::App(L"FATAL UNKNOWN EXCEPTION IN main.");
        return -3;
    }
    return 0;
}
