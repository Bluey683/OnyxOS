#pragma once
#include <string>

namespace onyx
{
namespace vm
{
    // Looks for the profile disk in the Xbox Dev Mode DevelopmentFiles area.
    std::wstring FindDevelopmentFilesVhdx(const std::string& profileId);
    // Performs basic VHDX container signature checks before passing a disk to QEMU.
    bool IsValidVhdxFile(const std::wstring& path);
    bool IsRecognizedIsoFile(const std::wstring& path);
    // A small marker is written only after the user confirms the guest installer finished.
    bool IsInstallationMarkedComplete(const std::string& profileId);
    bool MarkInstallationComplete(const std::string& profileId);
}
}
