#include <windows.h>
#include "LauncherPage.xaml.h"
#include <collection.h>
#include <ppltasks.h>
#include <windows.applicationmodel.activation.h>

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

ref class App sealed : public Application
{
public:
    App()
    {
        selection = nullptr;
        status = nullptr;
    }

protected:
    virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override
    {
        auto window = Window::Current;
        window->Content = ref new OnyxOS::LauncherPage();
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
            auto app = ref new App();
        }));
    return 0;
}
