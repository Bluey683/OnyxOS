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
        std::string cpuModel{ "max" };
        std::uint32_t memoryMB{ 3072 };
        std::uint32_t cpuCount{ 2 };
        bool useUefi{ true };
        bool enableNetwork{ true };
        bool enableVirtioGraphics{ true };
        bool enableUsbInput{ true };
        bool enabled{ true };
    };

    struct ValidationResult
    {
        bool valid{ false };
        std::string message;
    };

    // Builds arguments for the QEMU UWP host API. It does not launch a process.
    // Firmware and host-specific audio/video callbacks must be configured by
    // the QEMU integration layer after the matching runtime is loaded.
    ValidationResult Validate(const VmConfiguration& configuration);
    std::vector<std::string> BuildQemuArguments(const VmConfiguration& configuration);
}
