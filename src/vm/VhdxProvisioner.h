#pragma once
#include <cstdint>
#include <string>
namespace onyx::vm {
struct DiskProvisioningRequest { std::string profileId; std::wstring destinationPath; std::uint64_t sizeBytes{64ull*1024*1024*1024}; };
struct DiskProvisioningResult { bool success{false}; std::wstring message; };
DiskProvisioningResult CreateVhdxForInstall(const DiskProvisioningRequest& request);
}