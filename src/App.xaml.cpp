#include "App.xaml.h"
#include "LauncherPage.xaml.h"
#include "OnyxLogger.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace OnyxOS
{
    App::App()
    {
        OnyxLogger::App(L"App constructor entered.");
        InitializeComponent();
        UnhandledException += ref new UnhandledExceptionEventHandler(
            this, &App::OnUnhandledException);
        OnyxLogger::App(L"XAML application initialized.");
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
                OnyxLogger::App(L"Navigating to LauncherPage.");
                if (!frame->Navigate(LauncherPage::typeid))
                    OnyxLogger::App(L"ERROR: navigation to LauncherPage returned false.");
            }

            window->Activate();
            OnyxLogger::App(L"Main window activated.");
        }
        catch (Exception^ ex)
        {
            OnyxLogger::App(L"LAUNCH FAILURE: " + ex->Message);
            throw;
        }
    }
}

[MTAThread]
int main(Array<String^>^)
{
    OutputDebugStringW(L"[OnyxOS][APP] main() entered.\n");
    Application::Start(ref new ApplicationInitializationCallback(
        [](ApplicationInitializationCallbackParams^)
        {
            auto app = ref new OnyxOS::App();
        }));
    return 0;
}
