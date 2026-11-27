#include "VhdxProvisioner.h"

#include <windows.h>
#include <fileapifromapp.h>
#include <winrt/Windows.Storage.h>
#include <cstdint>
#include <string>

namespace onyx
{
namespace vm
{
    namespace
    {
        std::wstring LocalStatePath()
        {
            return winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().c_str();
        }

        std::wstring Windows11Path(const wchar_t* child)
        {
            return LocalStatePath() + L"\\Windows11\\" + child;
        }

        bool EnsureDirectory(const std::wstring& path)
        {
            if (CreateDirectoryFromAppW(path.c_str(), nullptr))
                return true;
            if (GetLastError() != ERROR_ALREADY_EXISTS)
                return false;
            WIN32_FILE_ATTRIBUTE_DATA data{};
            return GetFileAttributesExFromAppW(path.c_str(), GetFileExInfoStandard, &data) &&
                (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }

        bool ReadSignature(HANDLE file, std::uint64_t offset, const char* expected)
        {
            LARGE_INTEGER position{};
            position.QuadPart = static_cast<LONGLONG>(offset);
            if (!SetFilePointerEx(file, position, nullptr, FILE_BEGIN)) return false;
            char actual[4]{};
            DWORD read = 0;
            return ReadFile(file, actual, sizeof(actual), &read, nullptr) &&
                read == sizeof(actual) && actual[0] == expected[0] &&
                actual[1] == expected[1] && actual[2] == expected[2] &&
                actual[3] == expected[3];
        }

        bool IsRegularFile(const std::wstring& path)
        {
            WIN32_FILE_ATTRIBUTE_DATA data{};
            return GetFileAttributesExFromAppW(path.c_str(), GetFileExInfoStandard, &data) &&
                !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        }
    }

    // Creates the private LocalState layout used only by the Windows 11 guest.
    bool EnsureDevelopmentFilesDirectories()
    {
        const auto root = LocalStatePath();
        const std::wstring directories[] = {
            root + L"\\Windows11",
            Windows11Path(L"ISO"),
            Windows11Path(L"Disks"),
            Windows11Path(L"State"),
            Windows11Path(L"Configurations"),
            root + L"\\Logs"
        };
        for (const auto& directory : directories)
            if (!EnsureDirectory(directory)) return false;
        return true;
    }

    std::wstring FindDevelopmentFilesVhdx(const std::string& profileId)
    {
        if (profileId != "windows11" || !EnsureDevelopmentFilesDirectories()) return {};
        const auto path = Windows11Path(L"Disks\\Windows11.vhdx");
        return IsRegularFile(path) && IsValidVhdxFile(path) ? path : std::wstring{};
    }

    std::wstring FindDevelopmentFilesIso(const std::string& profileId)
    {
        if (profileId != "windows11" || !EnsureDevelopmentFilesDirectories()) return {};
        const auto path = Windows11Path(L"ISO\\Windows11.iso");
        return IsRecognizedIsoFile(path) ? path : std::wstring{};
    }

    bool IsValidVhdxFile(const std::wstring& path)
    {
        if (path.empty()) return false;
        HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        const bool largeEnough = GetFileSizeEx(file, &size) && size.QuadPart >= 3ll * 1024ll * 1024ll;
        const bool valid = largeEnough &&
            ReadSignature(file, 0, "vhdx") &&
            ReadSignature(file, 64ull * 1024ull, "head") &&
            ReadSignature(file, 128ull * 1024ull, "head") &&
            ReadSignature(file, 192ull * 1024ull, "regi");
        CloseHandle(file);
        return valid;
    }

    bool IsRecognizedIsoFile(const std::wstring& path)
    {
        if (path.empty()) return false;
        HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        bool valid = GetFileSizeEx(file, &size) && size.QuadPart >= 17ll * 2048ll;
        if (valid)
        {
            LARGE_INTEGER position{};
            position.QuadPart = 16ll * 2048ll + 1ll;
            valid = SetFilePointerEx(file, position, nullptr, FILE_BEGIN);
            char signature[5]{};
            DWORD read = 0;
            valid = valid && ReadFile(file, signature, sizeof(signature), &read, nullptr) &&
                read == sizeof(signature) && signature[0] == 'C' && signature[1] == 'D' &&
                signature[2] == '0' && signature[3] == '0' && signature[4] == '1';
        }
        CloseHandle(file);
        return valid;
    }

    bool IsInstallationMarkedComplete(const std::string& profileId)
    {
        if (profileId != "windows11") return false;
        return IsRegularFile(Windows11Path(L"State\\installation-complete.marker"));
    }

    bool MarkInstallationComplete(const std::string& profileId)
    {
        if (profileId != "windows11" || !EnsureDevelopmentFilesDirectories()) return false;
        const auto path = Windows11Path(L"State\\installation-complete.marker");
        HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        const char marker[] = "Windows 11 installation confirmed by user.\r\n";
        DWORD written = 0;
        const bool ok = WriteFile(file, marker, static_cast<DWORD>(sizeof(marker) - 1), &written, nullptr) &&
            written == sizeof(marker) - 1;
        CloseHandle(file);
        return ok;
    }

    void ClearInstallationMark(const std::string& profileId)
    {
        if (profileId == "windows11")
            DeleteFileFromAppW(Windows11Path(L"State\\installation-complete.marker").c_str());
    }
}
}
