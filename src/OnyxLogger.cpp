#include "OnyxLogger.h"
#include "OnyxNativeLog.h"

#include <windows.h>
#include <fileapifromapp.h>
#include <fileapifromapp.h>
#include <string>

using namespace Platform;
using namespace Windows::Storage;

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
    }

    void OnyxLogger::Write(String^ fileName, String^ source, String^ level, String^ message)
    {
        SYSTEMTIME now{};
        GetLocalTime(&now);

        wchar_t timestamp[64]{};
        swprintf_s(timestamp, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);

        String^ line = ref new String(timestamp) + L" [" + level + L"] [" + source + L"] " +
            (message == nullptr ? L"(empty message)" : message) + L"\r\n";
        OutputDebugStringW(line->Data());

        AcquireSRWLockExclusive(&g_logLock);
        HANDLE file = INVALID_HANDLE_VALUE;
        try
        {
            String^ localPath = ApplicationData::Current->LocalFolder->Path;
            String^ logDirectory = localPath + L"\\Logs";
            if (!CreateDirectoryFromAppW(logDirectory->Data(), nullptr) &&
                GetLastError() != ERROR_ALREADY_EXISTS)
            {
                OutputDebugStringW(L"[OnyxOS][ERROR] Could not create LocalState\\Logs.\n");
            }
            else
            {
                String^ fullPath = logDirectory + L"\\" + fileName;
                CREATEFILE2_EXTENDED_PARAMETERS createParams{};
                createParams.dwSize = sizeof(createParams);
                file = CreateFile2FromAppW(fullPath->Data(), FILE_APPEND_DATA,
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

                    if (!WriteAll(file, line->Data(), line->Length() * sizeof(wchar_t)))
                    {
                        wchar_t error[128]{};
                        swprintf_s(error, L"[OnyxOS] Cannot write log file (error %lu).\n", GetLastError());
                        OutputDebugStringW(error);
                    }
                    FlushFileBuffers(file);
                }
            }
        }
        catch (Exception^ ex)
        {
            OutputDebugStringW((L"[OnyxOS] Log storage exception: " + ex->Message + L"\n")->Data());
        }

        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
        ReleaseSRWLockExclusive(&g_logLock);
    }

    void OnyxLogger::App(String^ message) { Write(L"OnyxOS.log", L"APP", L"INFO", message); }
    void OnyxLogger::Info(String^ source, String^ message) { Write(L"OnyxOS.log", source, L"INFO", message); }
    void OnyxLogger::Warning(String^ source, String^ message) { Write(L"OnyxOS.log", source, L"WARN", message); }
    void OnyxLogger::Error(String^ source, String^ message) { Write(L"OnyxOS.log", source, L"ERROR", message); }
    void OnyxLogger::Qemu(String^ message)
    {
        Write(L"QEMU.log", L"QEMU", L"INFO", message);
        Write(L"OnyxOS.log", L"QEMU", L"INFO", message);
    }

    void WriteQemuLogNative(const wchar_t* message)
    {
        OnyxLogger::Qemu(ref new String(message ? message : L"(empty QEMU message)"));
    }
}
