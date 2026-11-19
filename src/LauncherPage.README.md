# OnyxOS Launcher XAML

This directory contains the first XAML UI scaffold for the OnyxOS Launcher.

## Files

- `LauncherPage.xaml` — responsive three-column profile selection screen for Windows 10, Windows 11, Debian, Ubuntu, Arch Linux, and experimental macOS.
- `LauncherPage.xaml.h` — C++/WinRT page declaration.
- `LauncherPage.xaml.cpp` — profile selection and safe placeholder launch handler.

## Current behavior

- Selecting a profile updates the selected-profile label and status message.
- The VM launch button remains disabled because QEMU has not yet been integrated.
- No ISO is opened, no virtual disk is created, and no guest operating system is started.

## Integration status

These files are a XAML/C++-WinRT page scaffold. The current CMake target still starts the older CoreWindow `IFrameworkView` host and does not yet run the XAML compiler or include this page in a packaged XAML application. Therefore this UI is **not yet wired into the app** and the current build workflow does not compile these XAML files.

The next integration step is to convert the UWP entry point to a XAML `Application` host, add the generated XAML build step/project metadata, and make the Launcher executable display this page. Do not mark the XAML launcher as running until that build and an Xbox deployment have been tested.
