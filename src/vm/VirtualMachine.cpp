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
        c.id = "windows11";
        c.name = "Windows 11";
        c.operatingSystem = GuestOperatingSystem::Windows;
        c.memoryMB = 3072;
        c.cpuCount = 1;
        c.tcgJitCacheMB = 128;
        c.displayWidth = 1920;
        c.displayHeight = 1080;
        c.windowsRelease = "Windows 11 22H2";
        c.enabled = (profileId == "windows11");
        return c;
    }

    ValidationResult Validate(const VmConfiguration& c)
    {
        if (!c.enabled || c.id != "windows11")
            return { false, "Somente o perfil Windows 11 está disponível." };
        if (!IsSafeIdentifier(c.id))
            return { false, "O ID da VM contém caracteres inválidos." };
        if (c.name != "Windows 11")
            return { false, "O nome do sistema convidado deve ser Windows 11." };
        if (c.diskPath.empty())
            return { false, "Informe o caminho do disco virtual Windows11-22H2.qcow2." };
        if (!IsSafeMachineType(c.machineType) || !IsSafeCpuModel(c.cpuModel))
            return { false, "O tipo de máquina ou modelo de CPU contém caracteres inválidos." };
        if (c.memoryMB < 1024 || c.memoryMB > 3072)
            return { false, "O limite de memória convidada é 3072 MB (3 GB)." };
        if (c.cpuCount != 1)
            return { false, "Este perfil permite exatamente uma vCPU." };
        if (c.tcgJitCacheMB < 16 || c.tcgJitCacheMB > 256)
            return { false, "O cache JIT TCG deve estar entre 16 MB e 256 MB." };
        if (c.displayWidth < 640 || c.displayWidth > 3840 || c.displayHeight < 480 ||
            c.displayHeight > 2160 || c.displayRefreshHz < 30 || c.displayRefreshHz > 120)
            return { false, "A resolução ou taxa de atualização do vídeo é inválida." };
        if (!c.isoPath.empty())
            return { true, "Windows 11 validado. A ISO será usada para instalação no disco QCOW2." };
        return { true, "Windows 11 validado. Inicialização pelo disco QCOW2." };
    }

    std::vector<std::string> BuildQemuArguments(const VmConfiguration& c)
    {
        if (!Validate(c).valid)
            return {};

        std::vector<std::string> args;
        const auto add = [&args](const char* option, const std::string& value)
        {
            args.emplace_back(option);
            args.push_back(value);
        };

        add("-name", "Windows 11");
        add("-machine", c.machineType);
        // QEMU TCG performs software CPU translation/JIT. Keep the translator
        // single-threaded and bound its translation-block cache for Xbox UWP.
        add("-accel", "tcg,thread=single,tb-size=" + std::to_string(c.tcgJitCacheMB));
        add("-cpu", c.cpuModel);
        add("-m", std::to_string(c.memoryMB));
        add("-smp", "1,sockets=1,cores=1,threads=1");
        add("-display", "host");
        add("-L", "qemu");
        // Use the packaged x86_64 UEFI firmware for the Windows 11 guest.
        add("-bios", c.useUefi ? "qemu/edk2-x86_64-code.fd" : "qemu/bios-256k.bin");

        add("-drive", "file=" + c.diskPath + ",format=qcow2,if=virtio,cache=writeback");

        if (!c.isoPath.empty())
        {
            add("-drive", "file=" + c.isoPath + ",if=none,id=onyx_iso,format=raw,media=cdrom,readonly=on");
            args.emplace_back("-device");
            args.emplace_back("ide-cd,drive=onyx_iso,bootindex=1");
        }
        add("-boot", "menu=on,strict=on");

        add("-audiodev", "xaudio2,id=audio0");
        args.emplace_back("-device");
        args.emplace_back("intel-hda");
        args.emplace_back("-device");
        args.emplace_back("hda-duplex,audiodev=audio0");

        if (c.enableVirtioGraphics) {
            args.emplace_back("-device");
            args.emplace_back("virtio-vga");
        }
        if (c.enableUsbInput) {
            args.emplace_back("-device");
            args.emplace_back("qemu-xhci");
            args.emplace_back("-device");
            args.emplace_back("usb-kbd");
            args.emplace_back("-device");
            args.emplace_back("usb-tablet");
        }
        if (c.enableNetwork) {
            add("-netdev", "user,id=onyxnet0");
            args.emplace_back("-device");
            args.emplace_back("e1000e,netdev=onyxnet0");
        }
        return args;
    }
}
}
