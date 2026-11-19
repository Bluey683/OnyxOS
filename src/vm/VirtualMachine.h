#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace onyx::vm
{
    enum class GuestOperatingSystem
    {
        Windows,
        Linux,
        MacOS
    };

    struct VmConfiguration
    {
        std::string id;
        std::string name;
        GuestOperatingSystem operatingSystem{ GuestOperatingSystem::Linux };
        std::string isoPath;
        std::string diskPath;
        std::string machineType{ "q35" };
        std::uint32_t memoryMB{ 3072 };
        std::uint32_t cpuCount{ 2 };
        bool useUefi{ true };
        bool enabled{ true };
    };

    struct ValidationResult
    {
        bool valid{ false };
        std::string message;
    };

    // Validates configuration values only. File existence and QEMU runtime
    // availability are checked by the platform integration layer.
    ValidationResult Validate(const VmConfiguration& configuration);

    // Produces QEMU command-line arguments for the future QEMU host adapter.
    // This function does not start a VM or execute a process.
    std::vector<std::string> BuildQemuArguments(const VmConfiguration& configuration);
}
