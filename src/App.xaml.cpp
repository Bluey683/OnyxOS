#include "App.xaml.h"
#include "OnyxLogger.h"
#include "vm/VirtualMachine.h"
#include "vm/QemuHostAdapter.h"
#include "vm/QemuInputBridge.h"
#include "vm/VhdxProvisioner.h"
#include "InstallerPage.xaml.h"
#include "GuestPage.xaml.h"
#include "graphics/QemuD3DRenderer.h"

#include <memory>
#include <string>
#include <vector>
#include <ppltasks.h>
#include <windows.h>
#include <fileapifromapp.h>
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
        std::shared_ptr<onyx::graphics::QemuD3DRenderer> renderer;
        std::shared_ptr<onyx::vm::QemuInputBridge> input;
        Windows::UI::Core::CoreIndependentInputSource^ pointerInput;
        bool profileSelected{ false };
        bool isoSelected{ false };
        bool diskSelected{ false };
        bool qemuStoppedForInstaller{ false };
        bool installerVmStarted{ false };
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
        OnyxLogger::Qemu(L"QEMU diagnostic log initialized. Waiting for VM launch request.");
        try
        {
            auto window = Window::Current;
            auto session = std::make_shared<VmSession>();
            auto root = ref new Grid();
            root->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 11, 16, 24));
            root->Padding = Thickness(32);

            auto layout = ref new StackPanel();
            layout->Spacing = 14;

            // Keep the entire launcher reachable with the Xbox controller.
            // A permanently visible vertical scrollbar gives users a clear
            // indication that more controls are available below the fold.
            auto mainScroll = ref new ScrollViewer();
            mainScroll->VerticalScrollBarVisibility = ScrollBarVisibility::Visible;
            mainScroll->VerticalScrollMode = ScrollMode::Enabled;
            mainScroll->HorizontalScrollBarVisibility = ScrollBarVisibility::Disabled;
            mainScroll->HorizontalScrollMode = ScrollMode::Disabled;
            mainScroll->ZoomMode = ZoomMode::Disabled;
            mainScroll->IsTabStop = true;

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

            if (!onyx::vm::EnsureDevelopmentFilesDirectories())
            {
                status->Text = L"Não foi possível criar as pastas em D:\\DevelopmentFiles\\OnyxOS. Verifique o acesso ao armazenamento DevelopmentFiles do Xbox Dev Mode.";
                OnyxLogger::App(L"Failed to create OnyxOS folders under D:\\DevelopmentFiles.");
            }
            else
            {
                OnyxLogger::App(L"DevelopmentFiles folders ready on D:.");
            }
            layout->Children->Append(status);

            auto guestPanel = ref new SwapChainPanel();
            guestPanel->Height = 420;
            guestPanel->MinHeight = 240;
            guestPanel->Visibility = Visibility::Collapsed;
            layout->Children->Append(guestPanel);

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

                        const auto bundledDisk = onyx::vm::FindDevelopmentFilesVhdx(profileId);
                        if (!bundledDisk.empty())
                        {
                            session->config.diskPath = ToUtf8(bundledDisk);
                            session->diskSelected = true;
                        }
                        const bool alreadyInstalled =
                            onyx::vm::IsInstallationMarkedComplete(profileId);
                        const auto existingIso = onyx::vm::FindDevelopmentFilesIso(profileId);
                        session->isoSelected = !alreadyInstalled && !existingIso.empty();
                        if (session->isoSelected)
                            session->config.isoPath = ToUtf8(existingIso);

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
                            (session->diskSelected
                                ? (alreadyInstalled
                                    ? L"\nVHDX encontrado em DevelopmentFiles. Sistema marcado como instalado; iniciará direto pelo disco."
                                    : (session->isoSelected ? L"\nVHDX e ISO encontrados em DevelopmentFiles. A instalação pode iniciar." : L"\nVHDX encontrado em DevelopmentFiles. Coloque a ISO em D:\\DevelopmentFiles\\OnyxOS\\ISO ou selecione manualmente."))
                                : L"\nColoque o VHDX em D:\\DevelopmentFiles\\OnyxOS\\VHDX ou selecione manualmente.");
                        status->Text = ref new String(details.c_str());
                        OnyxLogger::App(L"VM profile stored for QEMU launch: " + name);
                    });
                profiles->Children->Append(button);
            }

            layout->Children->Append(profiles);

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
                            std::wstring selectedIso(file->Path->Data());
                            if (!onyx::vm::IsRecognizedIsoFile(selectedIso))
                            {
                                session->isoSelected = false;
                                session->config.isoPath.clear();
                                const DWORD error = GetLastError();
                                status->Text = ref new String((L"ISO inacessível ou inválida (erro " + std::to_wstring(error) +
                                    L"). O Xbox não conseguiu ler o cabeçalho ISO9660.").c_str());
                                OnyxLogger::Qemu(L"ISO rejected or inaccessible through UWP file APIs.");
                                return;
                            }
                            CreateDirectoryFromAppW(L"D:\\DevelopmentFiles", nullptr);
                            CreateDirectoryFromAppW(L"D:\\DevelopmentFiles\\OnyxOS", nullptr);
                            CreateDirectoryFromAppW(L"D:\\DevelopmentFiles\\OnyxOS\\ISO", nullptr);
                            std::wstring isoName = L"D:\\DevelopmentFiles\\OnyxOS\\ISO\\";
                            isoName.append(session->config.id.begin(), session->config.id.end());
                            isoName += L".iso";
                            if (selectedIso != isoName &&
                                !CopyFileFromAppW(selectedIso.c_str(), isoName.c_str(), FALSE))
                            {
                                session->isoSelected = false;
                                status->Text = L"Não foi possível copiar a ISO para DevelopmentFiles. Verifique o acesso do Xbox Dev Mode à pasta.";
                                OnyxLogger::Qemu(L"ISO copy to DevelopmentFiles failed.");
                                return;
                            }
                            selectedIso = isoName;
                            session->config.isoPath = ToUtf8(selectedIso);
                            session->isoSelected = !session->config.isoPath.empty();
                            onyx::vm::ClearInstallationMark(session->config.id);
                            status->Text = L"ISO reconhecida: " + file->Name +
                                L"\\nA instalação/reinstalação será iniciada pelo ISO.";
                            OnyxLogger::Qemu(L"Recognized ISO selected; installation marker cleared for reinstall.");
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
                            std::wstring targetName;
                            if (session->config.id == "windows10") targetName = L"Windows10.vhdx";
                            else if (session->config.id == "windows11") targetName = L"Windows11.vhdx";
                            else if (session->config.id == "debian") targetName = L"Debian.vhdx";
                            else if (session->config.id == "ubuntu") targetName = L"Ubuntu.vhdx";
                            else if (session->config.id == "arch") targetName = L"ArchLinux.vhdx";
                            else if (session->config.id == "macos") targetName = L"MacOS.vhdx";

                            CreateDirectoryFromAppW(L"D:\\DevelopmentFiles", nullptr);
                            CreateDirectoryFromAppW(L"D:\\DevelopmentFiles\\OnyxOS", nullptr);
                            CreateDirectoryFromAppW(L"D:\\DevelopmentFiles\\OnyxOS\\VHDX", nullptr);
                            std::wstring selectedPath(file->Path->Data());
                            if (!targetName.empty())
                            {
                                std::wstring destination = L"D:\\DevelopmentFiles\\OnyxOS\\VHDX\\";
                                destination += targetName;
                                if (selectedPath != destination &&
                                    !CopyFileFromAppW(selectedPath.c_str(), destination.c_str(), FALSE))
                                {
                                    session->diskSelected = false;
                                    status->Text = L"Não foi possível copiar o VHDX para DevelopmentFiles. Verifique o acesso do Xbox Dev Mode à pasta.";
                                    OnyxLogger::Qemu(L"VHDX copy to DevelopmentFiles failed.");
                                    return;
                                }
                                selectedPath = destination;
                            }

                            if (!onyx::vm::IsValidVhdxFile(selectedPath))
                            {
                                session->diskSelected = false;
                                session->config.diskPath.clear();
                                status->Text = L"VHDX inválido ou inacessível. Verifique o arquivo antes de iniciar.";
                                OnyxLogger::Qemu(L"VHDX rejected by container signature validation.");
                                return;
                            }
                            session->config.diskPath = ToUtf8(selectedPath);
                            session->diskSelected = true;
                            status->Text = L"VHDX reconhecido: " + file->Name +
                                L"\\nDisco validado. O caminho de DevelopmentFiles será usado quando a cópia for permitida.";
                            OnyxLogger::Qemu(L"Validated VHDX selected: " + file->Name);
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
                [session, status, launchButton, stopButton, guestPanel, root, layout, mainScroll](Object^, RoutedEventArgs^)
                {
                    OnyxLogger::Qemu(L"Launch button clicked. Profile selected=" +
                        (session->profileSelected ? L"true" : L"false") +
                        L"; ISO selected=" + (session->isoSelected ? L"true" : L"false") +
                        L"; VHDX selected=" + (session->diskSelected ? L"true" : L"false") +
                        L"; profile=" + ref new String(ToWide(session->config.id).c_str()));
                    const bool alreadyInstalled =
                        onyx::vm::IsInstallationMarkedComplete(session->config.id);
                    if (!session->profileSelected || !session->diskSelected ||
                        (!alreadyInstalled && !session->isoSelected))
                    {
                        status->Text = L"Selecione um perfil e um VHDX válido. Para a primeira instalação, selecione também uma ISO.";
                        OnyxLogger::Qemu(L"Launch blocked: profile, VHDX or required ISO not selected.");
                        return;
                    }
                    if (!onyx::vm::IsValidVhdxFile(ToWide(session->config.diskPath)))
                    {
                        status->Text = L"O VHDX não passou na verificação de cabeçalho. A máquina não será iniciada.";
                        OnyxLogger::Qemu(L"Launch blocked: VHDX validation failed: " + ref new String(ToWide(session->config.diskPath).c_str()));
                        return;
                    }
                    if (alreadyInstalled)
                    {
                        session->config.isoPath.clear();
                        session->isoSelected = false;
                    }
                    else if (!onyx::vm::IsRecognizedIsoFile(ToWide(session->config.isoPath)))
                    {
                        status->Text = L"A ISO não foi reconhecida. Selecione uma ISO válida antes de instalar.";
                        OnyxLogger::Qemu(L"Launch blocked: ISO validation failed: " + ref new String(ToWide(session->config.isoPath).c_str()));
                        return;
                    }

                    const auto validation = onyx::vm::Validate(session->config);
                    if (!validation.valid)
                    {
                        status->Text = ref new String(ToWide(validation.message).c_str());
                        OnyxLogger::Qemu(L"Launch blocked by VM configuration validation: " + ref new String(ToWide(validation.message).c_str()));
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

                    InstallerPage^ installerPage = nullptr;
                    GuestPage^ guestPage = nullptr;
                    SwapChainPanel^ activeGuestPanel = guestPanel;
                    Windows::UI::Xaml::Controls::TextBox^ activeKeyboardSink = nullptr;
                    if (!alreadyInstalled)
                    {
                        installerPage = ref new InstallerPage();
                        installerPage->SetStatus(L"Inicializando QEMU pela ISO selecionada. Conclua o instalador do sistema e desligue a máquina antes de confirmar.");
                        activeGuestPanel = ref new SwapChainPanel();
                        activeKeyboardSink = installerPage->GetKeyboardSink();
                        activeGuestPanel->HorizontalAlignment = HorizontalAlignment::Stretch;
                        activeGuestPanel->VerticalAlignment = VerticalAlignment::Stretch;
                        installerPage->GetGuestHost()->Children->Append(activeGuestPanel);
                        root->Padding = Thickness(0);
                        root->Children->Clear();
                        root->Children->Append(installerPage);
                        session->qemuStoppedForInstaller = false;
                        session->installerVmStarted = false;
                        installerPage->StopRequested += ref new Windows::Foundation::EventHandler<Object^>(
                            [session, installerPage](Object^, Object^)
                            {
                                if (!session->installerVmStarted)
                                {
                                    installerPage->SetStatus(L"QEMU ainda não iniciou. Não é possível concluir a instalação.");
                                    return;
                                }
                                session->host->Stop();
                                session->qemuStoppedForInstaller = true;
                                installerPage->SetStopped();
                            });
                        installerPage->FinishRequested += ref new Windows::Foundation::EventHandler<Object^>(
                            [session, installerPage, root, layout, launchButton, mainScroll](Object^, Object^)
                            {
                                if (!session->qemuStoppedForInstaller || !session->installerVmStarted)
                                {
                                    installerPage->SetStatus(L"Execute o instalador, desligue a máquina e depois pare o QEMU antes de confirmar.");
                                    return;
                                }
                                if (!onyx::vm::MarkInstallationComplete(session->config.id))
                                {
                                    installerPage->SetStatus(L"Não foi possível gravar o estado em DevelopmentFiles. Verifique a permissão da pasta.");
                                    return;
                                }
                                session->config.isoPath.clear();
                                session->isoSelected = false;
                                session->qemuStoppedForInstaller = false;
                                session->installerVmStarted = false;
                                root->Children->Clear();
                                root->Padding = Thickness(32);
                                root->Padding = Thickness(32);
                                root->Children->Append(mainScroll);
                                launchButton->IsEnabled = true;
                                OnyxLogger::Qemu(L"Installation completion explicitly confirmed; next boot will use VHDX only.");
                            });
                    }
                    else
                    {
                        guestPage = ref new GuestPage();
                        guestPage->SetGuestProfile(
                            ref new String(ToWide(session->config.id).c_str()),
                            ref new String(ToWide(session->config.name).c_str()));
                        guestPage->SetRendererName(L"Direct3D 11 / HLSL");
                        activeGuestPanel = guestPage->GetQemuSurface();
                        activeKeyboardSink = guestPage->GetKeyboardSink();
                        root->Padding = Thickness(0);
                        root->Children->Clear();
                        root->Children->Append(guestPage);
                        guestPage->StopRequested += ref new Windows::Foundation::EventHandler<Object^>(
                            [session, guestPage, root, layout, launchButton, stopButton, status, mainScroll](Object^, Object^)
                            {
                                session->host->Stop();
                                guestPage->SetDisplayStatus(L"Máquina virtual parada.", false);
                                root->Children->Clear();
                                root->Padding = Thickness(32);
                                root->Children->Append(mainScroll);
                                launchButton->IsEnabled = true;
                                stopButton->IsEnabled = false;
                                status->Text = L"QEMU parado. A próxima inicialização usará o VHDX sem ISO.";
                            });
                    }

                    session->renderer = std::make_shared<onyx::graphics::QemuD3DRenderer>();
                    std::wstring rendererError;
                    if (!session->renderer->Initialize(activeGuestPanel, rendererError))
                    {
                        status->Text = L"Falha ao inicializar Direct3D/HLSL: " + ref new String(rendererError.c_str());
                        launchButton->IsEnabled = true;
                        if (installerPage)
                        {
                            installerPage->SetStatus(L"Falha ao iniciar Direct3D/HLSL. Voltando ao OnyxOS.");
                            session->installerVmStarted = false;
                            session->qemuStoppedForInstaller = false;
                        }
                        root->Children->Clear();
                        root->Padding = Thickness(32);
                                root->Children->Append(mainScroll);
                        launchButton->IsEnabled = true;
                        return;
                    }
                    // Route Xbox keyboard and pointer input from the guest surface into QEMU.
                    // CoreWindow receives physical/USB keyboard events even when the SwapChainPanel
                    // itself is not a focusable Control; pointer events are measured in panel pixels.
                    session->input = std::make_shared<onyx::vm::QemuInputBridge>(*session->host);
                    auto inputBridge = session->input;
                    // A tiny invisible TextBox is the actual XAML keyboard focus target.
                    // SwapChainPanel does not accept focus, so CoreWindow-only handlers
                    // can miss physical USB keyboard input while the guest page is active.
                    if (activeKeyboardSink != nullptr)
                    {
                        activeKeyboardSink->KeyDown += ref new Windows::UI::Xaml::Input::KeyEventHandler(
                            [inputBridge](Object^, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args)
                            {
                                inputBridge->SendVirtualKey(static_cast<int>(args->Key), true);
                                args->Handled = true;
                            });
                        activeKeyboardSink->KeyUp += ref new Windows::UI::Xaml::Input::KeyEventHandler(
                            [inputBridge](Object^, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args)
                            {
                                inputBridge->SendVirtualKey(static_cast<int>(args->Key), false);
                                args->Handled = true;
                            });
                        activeKeyboardSink->Focus(FocusState::Programmatic);
                        OnyxLogger::Qemu(L"Physical keyboard routed through focused XAML keyboard sink.");
                    }
                    // Use routed XAML pointer events on the guest surface.
                    // Do not run CoreIndependentInputSource::Dispatcher::ProcessEvents
                    // from a ThreadPool task: its dispatcher belongs to the UI thread,
                    // and pumping it from another thread can terminate the app at launch.
                    activeGuestPanel->PointerMoved += ref new Windows::UI::Xaml::Input::PointerEventHandler(
                        [inputBridge, activeGuestPanel](Object^, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args)
                        {
                            auto point = args->GetCurrentPoint(activeGuestPanel);
                            auto properties = point->Properties;
                            inputBridge->SendPointerPosition(point->Position.X, point->Position.Y,
                                activeGuestPanel->ActualWidth, activeGuestPanel->ActualHeight);
                            inputBridge->SendPointerButton(0, properties->IsLeftButtonPressed);
                            inputBridge->SendPointerButton(1, properties->IsMiddleButtonPressed);
                            inputBridge->SendPointerButton(2, properties->IsRightButtonPressed);
                        });
                    activeGuestPanel->PointerPressed += ref new Windows::UI::Xaml::Input::PointerEventHandler(
                        [inputBridge, activeGuestPanel](Object^, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args)
                        {
                            auto point = args->GetCurrentPoint(activeGuestPanel);
                            auto properties = point->Properties;
                            inputBridge->SendPointerPosition(point->Position.X, point->Position.Y,
                                activeGuestPanel->ActualWidth, activeGuestPanel->ActualHeight);
                            inputBridge->SendPointerButton(0, properties->IsLeftButtonPressed);
                            inputBridge->SendPointerButton(1, properties->IsMiddleButtonPressed);
                            inputBridge->SendPointerButton(2, properties->IsRightButtonPressed);
                            activeGuestPanel->CapturePointer(args->Pointer);
                        });
                    activeGuestPanel->PointerReleased += ref new Windows::UI::Xaml::Input::PointerEventHandler(
                        [inputBridge, activeGuestPanel](Object^, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args)
                        {
                            auto point = args->GetCurrentPoint(activeGuestPanel);
                            auto properties = point->Properties;
                            inputBridge->SendPointerButton(0, properties->IsLeftButtonPressed);
                            inputBridge->SendPointerButton(1, properties->IsMiddleButtonPressed);
                            inputBridge->SendPointerButton(2, properties->IsRightButtonPressed);
                        });
                    activeGuestPanel->PointerCaptureLost += ref new Windows::UI::Xaml::Input::PointerEventHandler(
                        [inputBridge](Object^, Windows::UI::Xaml::Input::PointerRoutedEventArgs^)
                        {
                            inputBridge->SendPointerButton(0, false);
                            inputBridge->SendPointerButton(1, false);
                            inputBridge->SendPointerButton(2, false);
                        });
                    OnyxLogger::Qemu(L"Keyboard, mouse and pointer input handlers attached to the active guest surface.");

                    // Enter immersive guest display automatically as soon as the VM page is shown.
                    // The small "Mostrar controles" button remains available to restore the toolbar.
                    if (installerPage) installerPage->EnterFullScreen();
                    if (guestPage) guestPage->EnterFullScreen();
                    activeGuestPanel->Visibility = Visibility::Visible;
                    auto arguments = onyx::vm::BuildQemuArguments(session->config);
                    arguments.insert(arguments.begin(), "qemu-system-x86_64");
                    auto dispatcher = status->Dispatcher;
                    auto renderer = session->renderer;
                    status->Text = L"QEMU iniciando em segundo plano: carregando BIOS, ISO e VHDX...";
                    OnyxLogger::Qemu(L"Starting QEMU asynchronously; waiting for the first guest framebuffer.");
                    auto onVideoFrame = [dispatcher, status, renderer, guestPage](const std::vector<std::uint32_t>& bgraFrame, int width, int height)
                    {
                        dispatcher->RunAsync(CoreDispatcherPriority::Normal,
                            ref new DispatchedHandler([status, renderer, guestPage, bgraFrame, width, height]()
                            {
                                std::wstring renderError;
                                if (!renderer->Present(bgraFrame, width, height, renderError))
                                    status->Text = L"Erro no renderer D3D11/HLSL: " + ref new String(renderError.c_str());
                                else
                                {
                                    status->Text = L"Framebuffer apresentado pelo Direct3D 11 com pixel shader HLSL.";
                                    if (guestPage)
                                        guestPage->SetDisplayStatus(L"Framebuffer recebido do QEMU e apresentado pelo Direct3D 11 / HLSL.", true);
                                }
                            }));
                    };

                    concurrency::create_task([session, arguments, onVideoFrame]()
                    {
                        return session->host->Start(arguments, onVideoFrame);
                    }).then([session, dispatcher, status, launchButton, stopButton, installerPage, guestPage, root, layout, mainScroll](bool started)
                    {
                        dispatcher->RunAsync(CoreDispatcherPriority::Normal,
                            ref new DispatchedHandler([session, status, launchButton, stopButton, installerPage, guestPage, root, layout, mainScroll, started]()
                            {
                                if (!started)
                                {
                                    status->Text = L"QEMU não iniciou: " + ref new String(session->host->Error().c_str());
                                    launchButton->IsEnabled = true;
                                    stopButton->IsEnabled = false;
                                    if (installerPage)
                                    {
                                        installerPage->SetStatus(L"QEMU não iniciou. Voltando ao OnyxOS; consulte QEMU.txt para ver o erro.");
                                        session->installerVmStarted = false;
                                        session->qemuStoppedForInstaller = false;
                                    }
                                    root->Children->Clear();
                                    root->Padding = Thickness(32);
                                root->Children->Append(mainScroll);
                                    return;
                                }

                                stopButton->IsEnabled = true;
                                if (guestPage)
                                    guestPage->SetStopEnabled(true);
                                if (installerPage)
                                {
                                    session->installerVmStarted = true;
                                    installerPage->SetStatus(L"Máquina virtual iniciada. O QEMU entregou o primeiro framebuffer; o instalador do sistema deve aparecer na tela.");
                                }
                                if (guestPage)
                                    guestPage->SetDisplayStatus(L"Máquina virtual iniciada. Primeiro framebuffer recebido do QEMU.", true);
                                status->Text = L"Máquina virtual iniciada: primeiro framebuffer recebido e enviado ao Direct3D 11 / HLSL.";
                                OnyxLogger::Qemu(L"QEMU initialized and first guest framebuffer received; VM startup confirmed.");
                            }));
                    });

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

            mainScroll->Content = layout;
            root->Padding = Thickness(32);
                                root->Children->Append(mainScroll);
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
