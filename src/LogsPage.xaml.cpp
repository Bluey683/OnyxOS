#include "LogsPage.xaml.h"
#include "OnyxLogger.h"

#include <ppltasks.h>

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace concurrency;

namespace OnyxOS
{
    LogsPage::LogsPage()
    {
        InitializeComponent();
        OnyxLogger::App(L"Diagnostics page opened.");
        RefreshLogs();
    }

    void LogsPage::RefreshLogs()
    {
        auto folder = ApplicationData::Current->LocalFolder;
        create_task(folder->CreateFileAsync(L"App.log", CreationCollisionOption::OpenIfExists))
            .then([](StorageFile^ file) { return FileIO::ReadTextAsync(file); })
            .then([this](task<String^> result)
            {
                try
                {
                    String^ value = result.get();
                    Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                        ref new Windows::UI::Core::DispatchedHandler([this, value]() { AppLogText->Text = value; }));
                }
                catch (Exception^ ex)
                {
                    OnyxLogger::App(L"Could not read App.log: " + ex->Message);
                }
            });

        create_task(folder->CreateFileAsync(L"QEMU.log", CreationCollisionOption::OpenIfExists))
            .then([](StorageFile^ file) { return FileIO::ReadTextAsync(file); })
            .then([this](task<String^> result)
            {
                try
                {
                    String^ value = result.get();
                    Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                        ref new Windows::UI::Core::DispatchedHandler([this, value]() { QemuLogText->Text = value; }));
                }
                catch (Exception^ ex)
                {
                    OnyxLogger::App(L"Could not read QEMU.log: " + ex->Message);
                }
            });
    }

    void LogsPage::BackButton_Click(Object^, RoutedEventArgs^)
    {
        auto frame = dynamic_cast<Frame^>(Window::Current->Content);
        if (frame && frame->CanGoBack) frame->GoBack();
    }

    void LogsPage::RefreshButton_Click(Object^, RoutedEventArgs^) { RefreshLogs(); }

    void LogsPage::CopyAppButton_Click(Object^, RoutedEventArgs^)
    {
        auto data = ref new DataPackage();
        data->SetText(AppLogText->Text);
        Clipboard::SetContent(data);
    }

    void LogsPage::CopyQemuButton_Click(Object^, RoutedEventArgs^)
    {
        auto data = ref new DataPackage();
        data->SetText(QemuLogText->Text);
        Clipboard::SetContent(data);
    }

    void LogsPage::ClearAppButton_Click(Object^, RoutedEventArgs^)
    {
        auto folder = ApplicationData::Current->LocalFolder;
        create_task(folder->CreateFileAsync(L"App.log", CreationCollisionOption::OpenIfExists))
            .then([](StorageFile^ file) { return FileIO::WriteTextAsync(file, L""); })
            .then([this](task<void> result)
            {
                try { result.get(); AppLogText->Text = L""; OnyxLogger::App(L"App log cleared."); }
                catch (Exception^ ex) { OnyxLogger::App(L"Failed to clear App.log: " + ex->Message); }
            });
    }

    void LogsPage::ClearQemuButton_Click(Object^, RoutedEventArgs^)
    {
        auto folder = ApplicationData::Current->LocalFolder;
        create_task(folder->CreateFileAsync(L"QEMU.log", CreationCollisionOption::OpenIfExists))
            .then([](StorageFile^ file) { return FileIO::WriteTextAsync(file, L""); })
            .then([this](task<void> result)
            {
                try { result.get(); QemuLogText->Text = L""; OnyxLogger::Qemu(L"QEMU log cleared."); }
                catch (Exception^ ex) { OnyxLogger::App(L"Failed to clear QEMU.log: " + ex->Message); }
            });
    }
}
