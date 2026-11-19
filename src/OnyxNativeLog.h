#pragma once

namespace OnyxOS
{
    // Native-safe bridge so the QEMU host adapter can write into the app log.
    void WriteQemuLogNative(const wchar_t* message);
}
