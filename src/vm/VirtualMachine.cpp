#include "VirtualMachine.h"

#include <algorithm>
#include <cctype>

namespace onyx
{
namespace vm
{
    namespace
    {
        bool IsSafeIdentifier(const std::string& value)
        {
            return !value.empty() && std::all_of(value.begin(), value.end(),
                [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; });
        }

        bool IsSafeMachineType(const std::string& value)
        {
            return !value.empty() && std::all_of(value.begin(), value.end(),
                [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_' || c == '.'; });
        }

        bool IsSafeCpuModel(const std::string& value)
        {
            return !value.empty() && std::all_of(value.begin(), value.end(),
                [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_' || c == '.'; });
        }
    }

    VmConfiguration CreateProfile(const std::string& profileId)
    {
        VmConfiguration c;
        c.id = profileId;
        c.enableNetwork = true;
        c.enableUsbInput = true;
        c.enableVirtioGraphics = true;
        c.displayWidth = 1280;
        c.displayHeight = 720;
        c.displayRefreshHz = 60;
        if (profileId == "windows10") {
            c.name = "Windows 10"; c.operatingSystem = GuestOperatingSystem::Windows;
            c.memoryMB = 3072; c.cpuCount = 2; c.displayWidth = 1366; c.displayHeight = 768;
        } else if (profileId == "windows11") {
            c.name = "Windows 11"; c.operatingSystem = GuestOperatingSystem::Windows;
            c.memoryMB = 4096; c.cpuCount = 2; c.displayWidth = 1920; c.displayHeight = 1080;
        } else if (profileId == "debian") {
            c.name = "Debian"; c.operatingSystem = GuestOperatingSystem::Linux;
            c.memoryMB = 2048; c.cpuCount = 2; c.displayWidth = 1280; c.displayHeight = 720;
        } else if (profileId == "ubuntu") {
            c.name = "Ubuntu"; c.operatingSystem = GuestOperatingSystem::Linux;
            c.memoryMB = 3072; c.cpuCount = 2; c.displayWidth = 1366; c.displayHeight = 768;
        } else if (profileId == "arch") {
            c.name = "Arch Linux"; c.operatingSystem = GuestOperatingSystem::Linux;
            c.memoryMB = 2048; c.cpuCount = 2; c.displayWidth = 1280; c.displayHeight = 720;
        } else if (profileId == "macos") {
            c.name = "macOS (experimental)"; c.operatingSystem = GuestOperatingSystem::MacOS;
            c.memoryMB = 4096; c.cpuCount = 2; c.displayWidth = 1280; c.displayHeight = 720;
            c.enabled = false;
        } else { c.enabled = false; }
        return c;
    }

    ValidationResult Validate(const VmConfiguration& configuration)
    {
        if (!configuration.enabled)
            return { false, "Este perfil de VM está desativado." };
        if (!IsSafeIdentifier(configuration.id))
            return { false, "O ID da VM deve conter apenas letras, números, hífen ou sublinhado." };
        if (configuration.name.empty())
            return { false, "Informe um nome para a máquina virtual." };
        if (configuration.diskPath.empty())
            return { false, "Informe o caminho do disco virtual." };
        if (!IsSafeMachineType(configuration.machineType))
            return { false, "O tipo de máquina QEMU contém caracteres inválidos." };
        if (!IsSafeCpuModel(configuration.cpuModel))
            return { false, "O modelo de CPU contém caracteres inválidos." };
        if (configuration.memoryMB < 512 || configuration.memoryMB > 262144)
            return { false, "A memória deve estar entre 512 MB e 262144 MB." };
        if (configuration.cpuCount < 1 || configuration.cpuCount > 64)
            return { false, "A quantidade de CPUs deve estar entre 1 e 64." };
        if (configuration.displayWidth < 640 || configuration.displayWidth > 3840 || configuration.displayHeight < 480 || configuration.displayHeight > 2160 || configuration.displayRefreshHz < 30 || configuration.displayRefreshHz > 120)
            return { false, "A resolução ou taxa de atualização do vídeo é inválida." };
        return { true, configuration.isoPath.empty()
            ? "Perfil validado. Inicialização pelo disco VHDX."
            : "Perfil validado. ISO e disco VHDX serão enviados à API QEMU UWP." };
    }

    std::vector<std::string> BuildQemuArguments(const VmConfiguration& configuration)
    {
        if (!Validate(configuration).valid)
            return {};

        std::vector<std::string> args;
        const auto add = [&args](const char* option, const std::string& value)
        {
            args.emplace_back(option);
            args.push_back(value);
        };

        add("-name", configuration.name);
        add("-machine", configuration.machineType);
        add("-accel", "tcg,thread=multi,tb-size=128"); // 128 MiB TCG JIT cache; avoids the oversized 208 MiB default on Xbox.
        add("-cpu", configuration.cpuModel);
        add("-m", std::to_string(configuration.memoryMB));
        add("-smp", std::to_string(configuration.cpuCount));
        add("-display", "host"); // QEMU-UWP-Host custom display backend feeds Direct3D.
        add("-L", "qemu"); // Packaged QEMU BIOS/firmware directory.
        // Use explicit drive backends and device boot indexes. The legacy
        // -cdrom shorthand left SeaBIOS trying the empty VHDX first on the
        // Xbox QEMU host, then reporting that the CD-ROM could not be read.
        // Keep the user ISO as a raw, read-only optical medium and force it
        // ahead of the virtual disk during installation.
        add("-drive", "file=" + configuration.diskPath + ",if=none,id=onyx_disk,format=vhdx");
        args.emplace_back("-device");
        args.emplace_back("virtio-blk-pci,drive=onyx_disk,bootindex=2");

        if (!configuration.isoPath.empty())
        {
            add("-drive", "file=" + configuration.isoPath + ",if=none,id=onyx_iso,format=raw,media=cdrom,readonly=on");
            args.emplace_back("-device");
            args.emplace_back("ide-cd,drive=onyx_iso,bootindex=1");
            add("-boot", "menu=on,strict=on");
        }
        else
        {
            add("-boot", "menu=on,strict=on");
        }

        // QEMU UWP host audio backend: route guest HDA output through XAudio2.
        add("-audiodev", "xaudio2,id=audio0");
        args.emplace_back("-device");
        args.emplace_back("intel-hda");
        args.emplace_back("-device");
        args.emplace_back("hda-duplex,audiodev=audio0");

        if (configuration.enableVirtioGraphics)
        {
            args.emplace_back("-device");
            args.emplace_back("virtio-vga");
        }

        if (configuration.enableUsbInput)
        {
            args.emplace_back("-device");
            args.emplace_back("qemu-xhci");
            args.emplace_back("-device");
            args.emplace_back("usb-kbd");
            args.emplace_back("-device");
            args.emplace_back("usb-tablet");
        }

        if (configuration.enableNetwork)
        {
            add("-netdev", "user,id=onyxnet0");
            args.emplace_back("-device");
            // e1000e has broad in-box driver support in Windows and Linux guests.
            args.emplace_back("e1000e,netdev=onyxnet0");
        }

        // UEFI firmware is deliberately not guessed. The runtime integration
        // must locate a compatible packaged firmware image before VM startup.
        return args;
    }
}
}
