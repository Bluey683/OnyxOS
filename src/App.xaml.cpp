#include "App.xaml.h"
#include "OnyxLogger.h"
#include "vm/VirtualMachine.h"
#include "vm/QemuHostAdapter.h"

#include <memory>
#include <string>
#include <vector>
#include <ppltasks.h>
#include <windows.h>
#include <windows.storage.h>
#include <windows.storage.pickers.h>

using namespace Platform;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Core;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using concurrency::create_task;
using concurrency::task_continuation_context;

namespace
{
    struct VmSession
    {
        onyx::vm::VmConfiguration config;
        std::shared_ptr<onyx::vm::QemuHostAdapter> host =
            std::make_shared<onyx::vm::QemuHostAdapter>();
        bool profileSelected{ false };
        bool isoSelected{ false };
        bool diskSelected{ false };
    };

    std::string ToUtf8(const std::wstring& value)
    {
        if (value.empty()) return {};
        const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        std::string result(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(),
            static_cast<int>(value.size()), &result[0], size, nullptr, nullptr);
        return result;
    }

    std::wstring ToWide(const std::string& value)
    {
        if (value.empty()) return {};
        const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(),
            static_cast<int>(value.size()), nullptr, 0);
        if (size <= 0) return {};
        std::wstring result(static_cast<size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(),
            static_cast<int>(value.size()), &result[0], size);
        return result;
    }
}

namespace OnyxOS
{
    App::App()
    {
        // Keep App.xaml out of the startup path: Xbox confirmed that its
        // InitializeComponent call fails before the application can launch.
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
        OnyxLogger::App(L"OnLaunched entered; building native C++ launcher UI.");
        try
        {
            auto window = Window::Current;
            auto session = std::make_shared<VmSession>();
            auto root = ref new Grid();
            root->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 11, 16, 24));
            root->Padding = Thickness(32);

            auto layout = ref new StackPanel();
            layout->Spacing = 14;

            auto heading = ref new TextBlock();
            heading->Text = L"ONYXOS";
            heading->FontSize = 34;
            heading->FontWeight = Windows::UI::Text::FontWeights::Bold;
            heading->Foreground = ref new SolidColorBrush(Colors::White);
            layout->Children->Append(heading);

            auto subtitle = ref new TextBlock();
            subtitle->Text = L"VIRTUAL MACHINE CENTER";
            subtitle->FontSize = 13;
            subtitle->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 139, 154, 175));
            layout->Children->Append(subtitle);

            auto prompt = ref new TextBlock();
            prompt->Text = L"Escolha seu sistema";
            prompt->FontSize = 25;
            prompt->Margin = Thickness(0, 12, 0, 0);
            prompt->Foreground = ref new SolidColorBrush(Colors::White);
            layout->Children->Append(prompt);

            auto hint = ref new TextBlock();
            hint->Text = L"Selecione um perfil, escolha a ISO e um disco VHDX existente.";
            hint->FontSize = 15;
            hint->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 154, 169, 188));
            layout->Children->Append(hint);

            auto status = ref new TextBlock();
            status->Text = L"Nenhum sistema selecionado.";
            status->FontSize = 16;
            status->TextWrapping = TextWrapping::Wrap;
            status->Margin = Thickness(0, 8, 0, 0);
            status->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 229, 184, 95));
            layout->Children->Append(status);

            auto profiles = ref new StackPanel();
            profiles->Spacing = 5;
            profiles->Margin = Thickness(0, 4, 0, 0);

            const wchar_t* names[] = {
                L"Windows 10", L"Windows 11", L"Debian",
                L"Ubuntu", L"Arch Linux", L"macOS (experimental)"
            };
            const char* profileIds[] = {
                "windows10", "windows11", "debian", "ubuntu", "arch", "macos"
            };

            for (int i = 0; i < 6; ++i)
            {
                String^ name = ref new String(names[i]);
                std::string profileId(profileIds[i]);
                auto button = ref new Button();
                button->Content = name;
                button->Tag = name;
                button->HorizontalAlignment = HorizontalAlignment::Stretch;
                button->HorizontalContentAlignment = HorizontalAlignment::Left;
                button->MinHeight = 42;
                button->Padding = Thickness(18, 6, 18, 6);
                button->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 20, 32, 49));
                button->Foreground = ref new SolidColorBrush(Colors::White);
                button->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 41, 59, 80));
                button->BorderThickness = Thickness(1);

                button->Click += ref new RoutedEventHandler(
                    [session, status, name, profileId](Object^, RoutedEventArgs^)
                    {
                        session->config = onyx::vm::CreateProfile(profileId);
                        session->profileSelected = session->config.enabled;
                        session->isoSelected = false;
                        session->diskSelected = false;
                        session->config.isoPath.clear();
                        session->config.diskPath.clear();

                        if (!session->config.enabled)
                        {
                            status->Text = L"Perfil experimental indisponível para inicialização.";
                            OnyxLogger::App(L"Disabled experimental profile selected: " + name);
                            return;
                        }

                        std::wstring details = L"Selecionado: " + std::wstring(name->Data()) +
                            L"\nVídeo: " + std::to_wstring(session->config.displayWidth) + L"x" +
                            std::to_wstring(session->config.displayHeight) + L" @ " +
                            std::to_wstring(session->config.displayRefreshHz) + L" Hz" +
                            L"\nMemória: " + std::to_wstring(session->config.memoryMB) +
                            L" MB | CPU: " + std::to_wstring(session->config.cpuCount) +
                            L"\nAgora escolha a ISO e o disco VHDX.";
                        status->Text = ref new String(details.c_str());
                        OnyxLogger::App(L"VM profile stored for QEMU launch: " + name);
                    });
                profiles->Children->Append(button);
            }

            auto scroll = ref new ScrollViewer();
            scroll->VerticalScrollBarVisibility = ScrollBarVisibility::Auto;
            scroll->Content = profiles;
            layout->Children->Append(scroll);

            auto pickers = ref new StackPanel();
            pickers->Orientation = Orientation::Horizontal;
            pickers->Spacing = 10;
            auto isoButton = ref new Button();
            isoButton->Content = L"Selecionar ISO";
            isoButton->MinWidth = 160;
            auto diskButton = ref new Button();
            diskButton->Content = L"Selecionar VHDX";
            diskButton->MinWidth = 160;
            pickers->Children->Append(isoButton);
            pickers->Children->Append(diskButton);
            layout->Children->Append(pickers);

            isoButton->Click += ref new RoutedEventHandler(
                [session, status](Object^, RoutedEventArgs^)
                {
                    if (!session->profileSelected)
                    {
                        status->Text = L"Primeiro selecione um sistema.";
                        return;
                    }
                    auto picker = ref new FileOpenPicker();
                    picker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
                    picker->FileTypeFilter->Append(L".iso");
                    create_task(picker->PickSingleFileAsync()).then(
                        [session, status](StorageFile^ file)
                        {
                            if (file == nullptr) return;
                            session->config.isoPath = ToUtf8(file->Path->Data());
                            session->isoSelected = !session->config.isoPath.empty();
                            status->Text = L"ISO selecionada: " + file->Name +
                                L"\nAgora selecione o disco VHDX.";
                            OnyxLogger::Qemu(L"ISO selected: " + file->Name);
                        }, task_continuation_context::use_current());
                });

            diskButton->Click += ref new RoutedEventHandler(
                [session, status](Object^, RoutedEventArgs^)
                {
                    if (!session->profileSelected)
                    {
                        status->Text = L"Primeiro selecione um sistema.";
                        return;
                    }
                    auto picker = ref new FileOpenPicker();
                    picker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
                    picker->FileTypeFilter->Append(L".vhdx");
                    create_task(picker->PickSingleFileAsync()).then(
                        [session, status](StorageFile^ file)
                        {
                            if (file == nullptr) return;
                            session->config.diskPath = ToUtf8(file->Path->Data());
                            session->diskSelected = !session->config.diskPath.empty();
                            status->Text = L"Disco selecionado: " + file->Name +
                                L"\nISO e VHDX prontos para validar e iniciar.";
                            OnyxLogger::Qemu(L"VHDX selected: " + file->Name);
                        }, task_continuation_context::use_current());
                });

            auto actions = ref new StackPanel();
            actions->Orientation = Orientation::Horizontal;
            actions->Spacing = 10;
            auto launchButton = ref new Button();
            launchButton->Content = L"Iniciar máquina virtual";
            launchButton->MinWidth = 220;
            auto stopButton = ref new Button();
            stopButton->Content = L"Parar QEMU";
            stopButton->MinWidth = 140;
            stopButton->IsEnabled = false;
            actions->Children->Append(launchButton);
            actions->Children->Append(stopButton);
            layout->Children->Append(actions);

            launchButton->Click += ref new RoutedEventHandler(
                [session, status, launchButton, stopButton](Object^, RoutedEventArgs^)
                {
                    if (!session->profileSelected || !session->isoSelected || !session->diskSelected)
                    {
                        status->Text = L"Selecione um perfil, uma ISO e um disco VHDX antes de iniciar.";
                        return;
                    }

                    const auto validation = onyx::vm::Validate(session->config);
                    if (!validation.valid)
                    {
                        status->Text = ref new String(ToWide(validation.message).c_str());
                        return;
                    }

                    launchButton->IsEnabled = false;
                    status->Text = L"Carregando a DLL QEMU e preparando os argumentos...";
                    OnyxLogger::Qemu(L"VM launch requested.");

                    if (!session->host->IsLoaded() && !session->host->Load())
                    {
                        status->Text = L"Falha ao carregar QEMU: " +
                            ref new String(session->host->Error().c_str());
                        launchButton->IsEnabled = true;
                        return;
                    }

                    auto arguments = onyx::vm::BuildQemuArguments(session->config);
                    arguments.insert(arguments.begin(), "qemu-system-x86_64");
                    auto dispatcher = status->Dispatcher;
                    const bool started = session->host->Start(arguments,
                        [dispatcher, status](const std::vector<std::uint32_t>& bgraFrame, int width, int height)
                        {
                            const std::wstring message = L"Framebuffer QEMU recebido e convertido para BGRA32: " +
                                std::to_wstring(width) + L"x" + std::to_wstring(height) +
                                L" (" + std::to_wstring(bgraFrame.size()) + L" pixels). Renderizador Direct3D/HLSL ainda não conectado.";
                            dispatcher->RunAsync(CoreDispatcherPriority::Normal,
                                ref new DispatchedHandler([status, message]()
                                {
                                    status->Text = ref new String(message.c_str());
                                }));
                        });

                    if (!started)
                    {
                        status->Text = L"Não foi possível iniciar a thread QEMU: " +
                            ref new String(session->host->Error().c_str());
                        launchButton->IsEnabled = true;
                        return;
                    }

                    stopButton->IsEnabled = true;
                    status->Text = L"QEMU solicitado. Aguardando inicialização e framebuffer...";
                    OnyxLogger::Qemu(L"QEMU worker thread started; awaiting guest initialization.");
                });

            stopButton->Click += ref new RoutedEventHandler(
                [session, status, launchButton, stopButton](Object^, RoutedEventArgs^)
                {
                    session->host->Stop();
                    stopButton->IsEnabled = false;
                    launchButton->IsEnabled = true;
                    status->Text = L"QEMU parado.";
                    OnyxLogger::Qemu(L"QEMU stopped by user.");
                });

            auto footer = ref new TextBlock();
            footer->Text = L"ONYXOS  |  QEMU HOST  |  ISO / VHDX / DIRECT3D";
            footer->FontSize = 11;
            footer->Margin = Thickness(0, 8, 0, 0);
            footer->Foreground = ref new SolidColorBrush(ColorHelper::FromArgb(255, 101, 117, 138));
            layout->Children->Append(footer);

            root->Children->Append(layout);
            window->Content = root;
            window->Activate();
            OnyxLogger::App(L"Native launcher UI activated; profile, ISO/VHDX pickers and QEMU controls wired.");
        }
        catch (Exception^ ex)
        {
            OnyxLogger::App(L"NATIVE LAUNCHER UI FAILURE: " + ex->Message);
            throw;
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
