#include "App.xaml.h"
#include "LauncherPage.xaml.h"

using namespace Platform;
using namespace Windows::UI::Xaml;

namespace OnyxOS
{
    App::App()
    {
        InitializeComponent();
    }

    void App::OnLaunched(
        Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^)
    {
        auto window = Window::Current;

        if (window->Content == nullptr)
        {
            window->Content = ref new LauncherPage();
        }

        window->Activate();
    }
}

[MTAThread]
int main(Array<String^>^)
{
    Application::Start(ref new ApplicationInitializationCallback(
        [](ApplicationInitializationCallbackParams^)
        {
            auto app = ref new OnyxOS::App();
        }));
    return 0;
}
