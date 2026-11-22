#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace onyx
{
namespace vm
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
        std::string diskFormat{ "vhdx" };
        std::string diskDevice{ "virtio-blk-pci" };
        std::string usbController{ "qemu-xhci" };
        std::string machineType{ "q35" };
        std::string cpuModel{ "max" };
        std::uint32_t memoryMB{ 3072 };
        std::uint32_t cpuCount{ 2 };
        bool useUefi{ true };
        bool enableNetwork{ true };
        bool enableVirtioGraphics{ true };
        bool enableUsbInput{ true };
        bool enableAudio{ true };
        bool enabled{ true };
        std::uint32_t displayWidth{ 1280 };
        std::uint32_t displayHeight{ 720 };
        std::uint32_t displayRefreshHz{ 60 };
    };

    struct ValidationResult
    {
        bool valid{ false };
        std::string message;
    };

    // Builds arguments for the QEMU UWP host API. It does not launch a process.
    VmConfiguration CreateProfile(const std::string& profileId);
    ValidationResult Validate(const VmConfiguration& configuration);
    std::vector<std::string> BuildQemuArguments(const VmConfiguration& configuration);
}
}
