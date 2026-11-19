#include "VirtualMachine.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace onyx::vm
{
    namespace
    {
        bool IsSafeIdentifier(const std::string& value)
        {
            return !value.empty() && std::all_of(value.begin(), value.end(),
                [](unsigned char c)
                {
                    return std::isalnum(c) || c == '-' || c == '_';
                });
        }

        bool IsSafeMachineType(const std::string& value)
        {
            return !value.empty() && std::all_of(value.begin(), value.end(),
                [](unsigned char c)
                {
                    return std::isalnum(c) || c == '-' || c == '_' || c == '.';
                });
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

        if (configuration.memoryMB < 512 || configuration.memoryMB > 262144)
            return { false, "A memória deve estar entre 512 MB e 262144 MB." };

        if (configuration.cpuCount < 1 || configuration.cpuCount > 64)
            return { false, "A quantidade de CPUs deve estar entre 1 e 64." };

        return { true, "Configuração válida. O runtime QEMU ainda precisa estar integrado." };
    }

    std::vector<std::string> BuildQemuArguments(const VmConfiguration& configuration)
    {
        const auto validation = Validate(configuration);
        if (!validation.valid)
            return {};

        std::vector<std::string> arguments;
        arguments.emplace_back("-name");
        arguments.push_back(configuration.name);

        arguments.emplace_back("-machine");
        arguments.push_back(configuration.machineType);

        arguments.emplace_back("-m");
        arguments.push_back(std::to_string(configuration.memoryMB));

        arguments.emplace_back("-smp");
        arguments.push_back(std::to_string(configuration.cpuCount));

        arguments.emplace_back("-drive");
        arguments.push_back("file=" + configuration.diskPath + ",if=virtio,format=vhdx");

        arguments.emplace_back("-cdrom");
        arguments.push_back(configuration.isoPath);

        arguments.emplace_back("-boot");
        arguments.emplace_back("order=d");

        if (configuration.useUefi)
        {
            // Firmware paths are intentionally not guessed. The QEMU adapter
            // must append -bios or pflash drives after locating bundled UEFI files.
        }

        return arguments;
    }
}
