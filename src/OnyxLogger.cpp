#include "OnyxLogger.h"
#include "OnyxNativeLog.h"

#include <windows.h>
#include <winrt/Windows.Storage.h>

#include <cwchar>
#include <string>

namespace OnyxOS
{
    namespace
    {
        SRWLOCK g_logLock = SRWLOCK_INIT;

        bool WriteAll(HANDLE file, const void* data, DWORD size)
        {
            const BYTE* bytes = static_cast<const BYTE*>(data);
            DWORD writtenTotal = 0;
            while (writtenTotal < size)
            {
                DWORD written = 0;
                if (!WriteFile(file, bytes + writtenTotal, size - writtenTotal, &written, nullptr) || written == 0)
                    return false;
                writtenTotal += written;
            }
            return true;
        }

        const wchar_t* Safe(const wchar_t* value)
        {
            return value ? value : L"(empty message)";
        }
    }

    void OnyxLogger::Write(const wchar_t* fileName, const wchar_t* source, const wchar_t* message)
    {
        SYSTEMTIME now{};
        GetLocalTime(&now);

        wchar_t timestamp[64]{};
        swprintf_s(timestamp, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);

        std::wstring line = timestamp;
        line += L" [";
        line += Safe(source);
        line += L"] ";
        line += Safe(message);
        line += L"\r\n";
        OutputDebugStringW(line.c_str());

        AcquireSRWLockExclusive(&g_logLock);
        HANDLE file = INVALID_HANDLE_VALUE;
        try
        {
            const auto folder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
            const std::wstring fullPath = std::wstring(folder.Path().c_str()) + L"\\" + Safe(fileName);

            CREATEFILE2_EXTENDED_PARAMETERS createParams{};
            createParams.dwSize = sizeof(createParams);
            file = CreateFile2FromAppW(fullPath.c_str(), FILE_APPEND_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_ALWAYS, &createParams);

            if (file == INVALID_HANDLE_VALUE)
            {
                wchar_t error[128]{};
                swprintf_s(error, L"[OnyxOS] Cannot open log file (error %lu).\n", GetLastError());
                OutputDebugStringW(error);
            }
            else
            {
                LARGE_INTEGER length{};
                if (GetFileSizeEx(file, &length) && length.QuadPart == 0)
                {
                    const wchar_t bom = 0xFEFF;
                    WriteAll(file, &bom, sizeof(bom));
                }

                const DWORD byteCount = static_cast<DWORD>(line.size() * sizeof(wchar_t));
                if (!WriteAll(file, line.data(), byteCount))
                {
                    wchar_t error[128]{};
                    swprintf_s(error, L"[OnyxOS] Cannot write log file (error %lu).\n", GetLastError());
                    OutputDebugStringW(error);
                }
                FlushFileBuffers(file);
            }
        }
        catch (const winrt::hresult_error& ex)
        {
            OutputDebugStringW((L"[OnyxOS] Log storage exception: " + std::wstring(ex.message().c_str()) + L"\n").c_str());
        }

        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
        ReleaseSRWLockExclusive(&g_logLock);
    }

    void OnyxLogger::App(const wchar_t* message)
    {
        Write(L"App.txt", L"APP", message);
    }

    void OnyxLogger::Qemu(const wchar_t* message)
    {
        Write(L"QEMU.txt", L"QEMU", message);
        Write(L"App.txt", L"QEMU", message);
    }

    void WriteQemuLogNative(const wchar_t* message)
    {
        OnyxLogger::Qemu(message);
    }
}
