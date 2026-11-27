#pragma once

#include <collection.h>

namespace OnyxOS
{
    public ref class OnyxLogger sealed
    {
    public:
        static void App(Platform::String^ message);
        static void Info(Platform::String^ source, Platform::String^ message);
        static void Warning(Platform::String^ source, Platform::String^ message);
        static void Error(Platform::String^ source, Platform::String^ message);
        static void Qemu(Platform::String^ message);
    private:
        static void Write(Platform::String^ fileName, Platform::String^ source,
            Platform::String^ level, Platform::String^ message);
    };
}
