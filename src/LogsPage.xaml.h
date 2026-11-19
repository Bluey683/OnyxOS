#pragma once

#include "src/LogsPage.g.h"

namespace OnyxOS
{
    public ref class LogsPage sealed
    {
    public:
        LogsPage();

    private:
        void RefreshLogs();
        void BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
        void RefreshButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
        void CopyAppButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
        void CopyQemuButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
        void ClearAppButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
        void ClearQemuButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
    };
}
