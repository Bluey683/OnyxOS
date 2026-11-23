#pragma once
#include <string>

namespace onyx::vm {
    // Ensures LocalState/Disks/windows7.qcow2 exists and has QCOW2 magic.
    // Downloads the configured Archive.org image when it is missing.
    bool PrepareWindows7Disk(const std::wstring& localStatePath,
                             std::wstring& diskPath,
                             std::wstring& error) noexcept;
}
