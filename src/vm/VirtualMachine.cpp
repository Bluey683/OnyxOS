#include "VirtualMachine.h"

#include <algorithm>
#include <cctype>

namespace onyx::vm
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

    ValidationResult Validate(const VmConfiguration& configuration)
    {
        if (!configuration.enabled)
            return { false, "Este perfil de VM está desativado." };
        if (!IsSafeIdentifier(configuration.id))
            return { false, "O ID da VM deve conter apenas letras, números, hífen ou sublinhado." };
        if (configuration.name.empty())
            return { false, "Informe um nome para a máquina virtual." };
        if (configuration.isoPath.empty())
            return { false, "Selecione uma imagem ISO." };
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
        return { true, "Perfil validado. A inicialização ainda depende da integração da API QEMU UWP." };
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
        add("-accel", "tcg,thread=multi");
        add("-cpu", configuration.cpuModel);
        add("-m", std::to_string(configuration.memoryMB));
        add("-smp", std::to_string(configuration.cpuCount));
        add("-drive", "file=" + configuration.diskPath + ",if=virtio,format=vhdx");
        add("-cdrom", configuration.isoPath);
        add("-boot", "order=d,menu=on");

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
            args.emplace_back("virtio-net-pci,netdev=onyxnet0");
        }

        // UEFI firmware is deliberately not guessed. The runtime integration
        // must locate a compatible packaged firmware image before VM startup.
        return args;
    }
}
