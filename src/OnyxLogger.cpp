#include "OnyxLogger.h"
#include "OnyxNativeLog.h"

#include <ppltasks.h>
#include <windows.h>

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace concurrency;

namespace OnyxOS
{
    void OnyxLogger::Write(String^ fileName, String^ source, String^ message)
    {
        String^ line = DateTime::Now.ToString() + L" [" + source + L"] " + message + L"\r\n";
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

    void OnyxLogger::App(String^ message)
    {
        Write(L"App.log", L"APP", message);
    }

    void OnyxLogger::Qemu(String^ message)
    {
        Write(L"QEMU.log", L"QEMU", message);
    }

    void WriteQemuLogNative(const wchar_t* message)
    {
        OnyxLogger::Qemu(ref new String(message ? message : L"(empty QEMU message)"));
    }
}
