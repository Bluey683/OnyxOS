#include <windows.h>
#include <windows.applicationmodel.activation.h>
#include "LauncherPage.xaml.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI;

ref class App sealed : public Application
{
protected:
    virtual void OnLaunched(
        Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override
    {
        auto window = Window::Current;

        try
        {
            window->Content = ref new OnyxOS::LauncherPage();
        }
        catch (Platform::Exception^ error)
        {
            auto panel = ref new Grid();
            panel->Background = ref new Media::SolidColorBrush(ColorHelper::FromArgb(255, 11, 16, 24));

            auto message = ref new TextBlock();
            message->Text = L"ONYXOS NÃO CONSEGUIU CARREGAR A INTERFACE\n\n" + error->Message;
            message->Foreground = ref new Media::SolidColorBrush(Colors::White);
            message->FontSize = 22;
            message->TextWrapping = TextWrapping::Wrap;
            message->Margin = Thickness(36);
            message->VerticalAlignment = VerticalAlignment::Center;
            message->HorizontalAlignment = HorizontalAlignment::Center;

            panel->Children->Append(message);
            window->Content = panel;
        }

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
