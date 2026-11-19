#include <windows.h>
#include <windows.applicationmodel.activation.h>
#include "LauncherPage.xaml.h"

using namespace Platform;
using namespace Windows::UI::Xaml;

ref class App sealed : public Application
{
protected:
    virtual void OnLaunched(
        Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override
    {
        auto window = Window::Current;
        window->Content = ref new OnyxOS::LauncherPage();
        window->Activate();
    }
};

[MTAThread]
int main(Array<String^>^)
{
    Application::Start(ref new ApplicationInitializationCallback(
        [](ApplicationInitializationCallbackParams^)
        {
            auto app = ref new App();
        }));
    return 0;
}
