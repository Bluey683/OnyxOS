#pragma once
#include <string>

namespace onyx
{
namespace vm
{
    // Creates and resolves the Windows 11 ISO, QCOW2 and state folders in app-private LocalState.
    bool EnsureDevelopmentFilesDirectories();
    std::wstring FindDevelopmentFilesDisk(const std::string& profileId);
    std::wstring FindDevelopmentFilesIso(const std::string& profileId);
    // Checks the QCOW2 magic header before passing a disk image to QEMU.
    bool IsValidQcow2File(const std::wstring& path);
    bool IsRecognizedIsoFile(const std::wstring& path);
    // A small marker is written only after the user confirms the guest installer finished.
    bool IsInstallationMarkedComplete(const std::string& profileId);
    bool MarkInstallationComplete(const std::string& profileId);
    void ClearInstallationMark(const std::string& profileId);
}
}
