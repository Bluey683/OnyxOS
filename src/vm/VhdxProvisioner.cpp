#include "VhdxProvisioner.h"

#include <windows.h>
#include <fileapifromapp.h>
#include <cstdint>
#include <string>

namespace onyx
{
namespace vm
{
    namespace
    {
        const wchar_t* DiskNameForProfile(const std::string& id)
        {
            if (id == "windows10") return L"Windows10.vhdx";
            if (id == "windows11") return L"Windows11.vhdx";
            if (id == "debian") return L"Debian.vhdx";
            if (id == "ubuntu") return L"Ubuntu.vhdx";
            if (id == "arch") return L"ArchLinux.vhdx";
            if (id == "macos") return L"MacOS.vhdx";
            return nullptr;
        }

        std::wstring StatePathForProfile(const std::string& id)
        {
            if (id.empty()) return {};
            std::wstring path = L"D:\\DevelopmentFiles\\OnyxOS\\State\\";
            path.append(id.begin(), id.end());
            path += L".installed";
            return path;
        }

        bool ReadSignature(HANDLE file, std::uint64_t offset, const char* expected)
        {
            LARGE_INTEGER position;
            position.QuadPart = static_cast<LONGLONG>(offset);
            if (!SetFilePointerEx(file, position, nullptr, FILE_BEGIN)) return false;
            char actual[8] = {};
            DWORD read = 0;
            return ReadFile(file, actual, 4, &read, nullptr) &&
                read == 4 && actual[0] == expected[0] &&
                actual[1] == expected[1] && actual[2] == expected[2] &&
                actual[3] == expected[3];
        }
    }

    bool EnsureDevelopmentFilesDirectories()
    {
        const wchar_t* directories[] = {
            L"D:\\DevelopmentFiles",
            L"D:\\DevelopmentFiles\\OnyxOS",
            L"D:\\DevelopmentFiles\\OnyxOS\\VHDX",
            L"D:\\DevelopmentFiles\\OnyxOS\\ISO",
            L"D:\\DevelopmentFiles\\OnyxOS\\State"
        };
        for (const auto directory : directories)
        {
            if (CreateDirectoryFromAppW(directory, nullptr))
                continue;
            if (GetLastError() != ERROR_ALREADY_EXISTS)
                return false;
            WIN32_FILE_ATTRIBUTE_DATA attributesData{};
            const bool attributesOk = GetFileAttributesExFromAppW(directory, GetFileExInfoStandard, &attributesData) != FALSE;
            const DWORD attributes = attributesOk ? attributesData.dwFileAttributes : INVALID_FILE_ATTRIBUTES;
            if (attributes == INVALID_FILE_ATTRIBUTES ||
                !(attributes & FILE_ATTRIBUTE_DIRECTORY))
                return false;
        }
        return true;
    }

    std::wstring FindDevelopmentFilesVhdx(const std::string& profileId)
    {
        const wchar_t* filename = DiskNameForProfile(profileId);
        if (!filename) return {};

        const wchar_t* roots[] = {
            L"D:\\DevelopmentFiles\\OnyxOS\\VHDX\\",
            L"D:\\DevelopmentFiles\\OnyxOS\\",
            L"D:\\DevelopmentFiles\\"
        };
        for (const auto root : roots)
        {
            std::wstring candidate(root);
            candidate += filename;
            WIN32_FILE_ATTRIBUTE_DATA attributesData{};
            const bool attributesOk = GetFileAttributesExFromAppW(candidate.c_str(), GetFileExInfoStandard, &attributesData) != FALSE;
            const DWORD attributes = attributesOk ? attributesData.dwFileAttributes : INVALID_FILE_ATTRIBUTES;
            if (attributes != INVALID_FILE_ATTRIBUTES &&
                !(attributes & FILE_ATTRIBUTE_DIRECTORY) &&
                IsValidVhdxFile(candidate))
                return candidate;
        }
        return {};
    }

    bool IsValidVhdxFile(const std::wstring& path)
    {
        if (path.empty()) return false;
        HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            OPEN_EXISTING, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;

        LARGE_INTEGER size = {};
        const bool largeEnough = GetFileSizeEx(file, &size) &&
            size.QuadPart >= 3ll * 1024ll * 1024ll;
        const bool valid = largeEnough &&
            ReadSignature(file, 0, "vhdx") &&
            ReadSignature(file, 64ull * 1024ull, "head") &&
            ReadSignature(file, 128ull * 1024ull, "head") &&
            ReadSignature(file, 192ull * 1024ull, "regi");
        CloseHandle(file);
        return valid;
    }

    std::wstring FindDevelopmentFilesIso(const std::string& profileId)
    {
        const wchar_t* filename = nullptr;
        if (profileId == "windows10") filename = L"Windows10.iso";
        else if (profileId == "windows11") filename = L"Windows11.iso";
        else if (profileId == "debian") filename = L"Debian.iso";
        else if (profileId == "ubuntu") filename = L"Ubuntu.iso";
        else if (profileId == "arch") filename = L"ArchLinux.iso";
        else if (profileId == "macos") filename = L"MacOS.iso";
        if (!filename) return {};

        const std::wstring roots[] = {
            L"D:\\DevelopmentFiles\\OnyxOS\\ISO\\",
            L"D:\\DevelopmentFiles\\OnyxOS\\",
            L"D:\\DevelopmentFiles\\"
        };
        for (const auto& root : roots)
        {
            const std::wstring candidate = root + filename;
            if (IsRecognizedIsoFile(candidate))
                return candidate;
        }

        WIN32_FIND_DATAW data{};
        const std::wstring pattern = L"D:\\DevelopmentFiles\\OnyxOS\\ISO\\*.iso";
        HANDLE search = FindFirstFileExFromAppW(pattern.c_str(), FindExInfoBasic,
            &data, FindExSearchNameMatch, nullptr, 0);
        if (search != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    const std::wstring candidate = roots[0] + data.cFileName;
                    if (IsRecognizedIsoFile(candidate))
                    {
                        FindClose(search);
                        return candidate;
                    }
                }
            } while (FindNextFileW(search, &data));
            FindClose(search);
        }
        return {};
    }

    bool IsRecognizedIsoFile(const std::wstring& path)
    {
        if (path.empty()) return false;
        HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            OPEN_EXISTING, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size = {};
        bool valid = GetFileSizeEx(file, &size) && size.QuadPart >= 17ll * 2048ll;
        if (valid)
        {
            LARGE_INTEGER position;
            position.QuadPart = 16ll * 2048ll + 1ll;
            valid = SetFilePointerEx(file, position, nullptr, FILE_BEGIN);
            char signature[5] = {};
            DWORD read = 0;
            valid = valid && ReadFile(file, signature, 5, &read, nullptr) &&
                read == 5 && signature[0] == 'C' && signature[1] == 'D' &&
                signature[2] == '0' && signature[3] == '0' && signature[4] == '1';
        }
        CloseHandle(file);
        return valid;
    }

    bool IsInstallationMarkedComplete(const std::string& profileId)
    {
        const auto path = StatePathForProfile(profileId);
        if (path.empty()) return false;
        WIN32_FILE_ATTRIBUTE_DATA attributesData{};
        const DWORD attributes = GetFileAttributesExFromAppW(path.c_str(), GetFileExInfoStandard, &attributesData)
            ? attributesData.dwFileAttributes : INVALID_FILE_ATTRIBUTES;
        return attributes != INVALID_FILE_ATTRIBUTES &&
            !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool MarkInstallationComplete(const std::string& profileId)
    {
        const auto path = StatePathForProfile(profileId);
        if (path.empty()) return false;
        CreateDirectoryFromAppW(L"D:\\DevelopmentFiles\\OnyxOS", nullptr);
        CreateDirectoryFromAppW(L"D:\\DevelopmentFiles\\OnyxOS\\State", nullptr);
        HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        const char marker[] = "OnyxOS installation confirmed by user.\r\n";
        DWORD written = 0;
        const bool ok = WriteFile(file, marker,
            static_cast<DWORD>(sizeof(marker) - 1), &written, nullptr) &&
            written == sizeof(marker) - 1;
        CloseHandle(file);
        return ok;
    }
    void ClearInstallationMark(const std::string& profileId)
    {
        const auto path = StatePathForProfile(profileId);
        if (!path.empty()) DeleteFileFromAppW(path.c_str());
    }

}
}
