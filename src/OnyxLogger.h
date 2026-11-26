#pragma once

namespace OnyxOS
{
    class OnyxLogger final
    {
    public:
        static void App(const wchar_t* message);
        static void Qemu(const wchar_t* message);

    private:
        static void Write(const wchar_t* fileName, const wchar_t* source, const wchar_t* message);
    };
}
