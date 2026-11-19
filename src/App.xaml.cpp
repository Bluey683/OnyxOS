#include "App.xaml.h"
#include "MainPage.xaml.h"
#include "OnyxLogger.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace OnyxOS
{
    App::App()
    {
        OnyxLogger::App(L"App constructor entered.");
        try
        {
            InitializeComponent();
            OnyxLogger::App(L"Application XAML initialized.");
        }
        catch (Exception^ ex)
        {
            OnyxLogger::App(L"APP XAML INITIALIZATION FAILURE: " + ex->Message);
            throw;
        }

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
        OnyxLogger::App(L"OnLaunched entered.");
        try
        {
            auto window = Window::Current;
            auto frame = dynamic_cast<Frame^>(window->Content);
            if (frame == nullptr)
            {
                OnyxLogger::App(L"Creating root navigation frame.");
                frame = ref new Frame();
                window->Content = frame;
            }

            if (frame->Content == nullptr)
            {
                OnyxLogger::App(L"Navigating to MainPage.");
                if (!frame->Navigate(MainPage::typeid))
                    OnyxLogger::App(L"ERROR: navigation to MainPage returned false.");
            }

            window->Activate();
            OnyxLogger::App(L"Main window activated.");
        }
        catch (Exception^ ex)
        {
            // Do not let a page/XAML navigation failure terminate the process.
            // Keep a visible diagnostic screen so the Xbox app does not return
            // to Home immediately and the failure can be identified.
            OnyxLogger::App(L"LAUNCH FAILURE: " + ex->Message);
            OutputDebugStringW((L"[OnyxOS] LAUNCH FAILURE: " + ex->Message + L"\\n")->Data());

            try
            {
                auto fallback = ref new Grid();
                fallback->Background = ref new Windows::UI::Xaml::Media::SolidColorBrush(
                    Windows::UI::Colors::Black);

                auto message = ref new TextBlock();
                message->Text = L"OnyxOS iniciou, mas a tela principal falhou.\\n\\n" + ex->Message;
                message->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(
                    Windows::UI::Colors::White);
                message->FontSize = 22;
                message->TextWrapping = Windows::UI::Xaml::TextWrapping::Wrap;
                message->Margin = Windows::UI::Xaml::Thickness(32);
                message->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Center;

                fallback->Children->Append(message);
                Window::Current->Content = fallback;
                Window::Current->Activate();
            }
            catch (...)
            {
                OutputDebugStringW(L"[OnyxOS] Failed to create fallback diagnostic screen.\\n");
            }
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
                // ApplicationData::Current is accessed only after UWP activation.
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
