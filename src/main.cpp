#include "App.h"

#include <windows.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/base.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    winrt::Windows::ApplicationModel::Core::CoreApplication::Run(
        winrt::make<winrt::OnyxOS::implementation::App>());
    return 0;
}
