#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace onyx
{
namespace vm
{
    enum class GuestOperatingSystem { Windows };

    struct VmConfiguration
    {
        std::string id;
        std::string name;
        GuestOperatingSystem operatingSystem{ GuestOperatingSystem::Windows };
        std::string isoPath;
        std::string diskPath;
        std::string machineType{ "q35" };
        std::string cpuModel{ "max" };
        std::uint32_t memoryMB{ 3072 };
        std::uint32_t cpuCount{ 1 };
        std::uint32_t tcgJitCacheMB{ 128 };
        bool useUefi{ true };
        bool enableNetwork{ true };
        bool enableVirtioGraphics{ true };
        bool enableUsbInput{ true };
        bool enabled{ true };
        std::uint32_t displayWidth{ 1920 };
        std::uint32_t displayHeight{ 1080 };
        std::uint32_t displayRefreshHz{ 60 };
        std::string windowsRelease{ "Windows 11 22H2" };
    };

    struct ValidationResult
    {
        bool valid{ false };
        std::string message;
    };

    VmConfiguration CreateProfile(const std::string& profileId);
    ValidationResult Validate(const VmConfiguration& configuration);
    std::vector<std::string> BuildQemuArguments(const VmConfiguration& configuration);
}
}
