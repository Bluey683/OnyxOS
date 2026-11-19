#include "OnyxLogger.h"
#include "OnyxNativeLog.h"

#include <ppltasks.h>
#include <windows.h>

using namespace Platform;
using namespace Windows::Storage;
using namespace concurrency;

namespace OnyxOS
{
    void OnyxLogger::Write(String^ fileName, String^ source, String^ message)
    {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        wchar_t timestamp[64]{};
        swprintf_s(timestamp, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
        String^ line = ref new String(timestamp) + L" [" + source + L"] " + message + L"\r\n";
        OutputDebugStringW(line->Data());

        try
        {
            auto folder = ApplicationData::Current->LocalFolder;
            create_task(folder->CreateFileAsync(fileName, CreationCollisionOption::OpenIfExists))
                .then([line](StorageFile^ file)
                {
                    return FileIO::AppendTextAsync(file, line);
                })
                .then([](task<void> result)
                {
                    try { result.get(); }
                    catch (Exception^) { OutputDebugStringW(L"[OnyxOS] Failed to persist log entry.\n"); }
                });
        }
        catch (Exception^)
        {
            OutputDebugStringW(L"[OnyxOS] Could not access LocalState log folder.\n");
        }
    }

    void OnyxLogger::App(String^ message) { Write(L"App.log", L"APP", message); }
    void OnyxLogger::Qemu(String^ message) { Write(L"QEMU.log", L"QEMU", message); }

    void WriteQemuLogNative(const wchar_t* message)
    {
        OnyxLogger::Qemu(ref new String(message ? message : L"(empty QEMU message)"));
    }
}
