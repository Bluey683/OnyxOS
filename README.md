# VMware-UWP-Xbox

A Visual Studio 2026 / C++ project for an Xbox Dev Mode UWP application inspired by the VMware Workstation 12 desktop interface.

**Initial scope:** VMware-style application shell and exactly one guest profile: **Windows x86 (32-bit)**. This repository does not claim to port VMware Workstation 12's proprietary hypervisor or to boot a guest OS yet.

## Target

- Xbox One / Xbox Series X|S Dev Mode, subject to API and package validation.
- Visual Studio 2026, C++ / MSVC, x64 app package.
- Compatible Windows SDK; 10.0.26100.0 is the intended baseline.
- VMware Workstation 12-inspired full-screen interface.
- Added a native Win32 window-host class (`src/vmware/Win32Window.*`) and registered it in the Visual Studio project; actual desktop top-level HWND creation is compiled only for the desktop API partition.
- Console surface/window using Win32 only if the Xbox app model supports a second top-level window; otherwise use a second in-process view/surface.
- XAML UI removed: no `.xaml`, `.xaml.h`, or `.xaml.cpp` page files remain in the project.
- Native C++/WinRT `IFrameworkView` / `CoreWindow` startup replaces the XAML `Application` startup.
- Native keyboard routing and controller-driven virtual mouse.

## The only guest profile

- **Windows 32-bit (x86)**.
- No Linux, macOS, Windows x64, or other guest profiles in the initial release.
- No Windows ISO or installer is bundled. The user supplies compatible installation media if a fresh guest installation is needed and is responsible for licensing.

## Storage

The app creates its own folders under package `LocalState/VMware/`:

```text
LocalState/VMware/
├── Config/
├── Virtual Machines/
├── ISO/
├── Disks/
├── Snapshots/
├── Logs/
└── Cache/
```

The app must not create or modify the user's Windows Documents, Desktop, Downloads, or other personal folders.

## Technical limits

Xbox UWP is not unrestricted desktop Windows. Separate top-level Win32 windows, child processes, drivers, unrestricted file access and VT-x must not be assumed available. A 32-bit guest does not automatically eliminate the need for hardware virtualization; actual guest execution requires a compatible software execution engine or supported hardware virtualization path.

The original VMware Workstation 12 host application and its drivers cannot simply be copied into an APPX. The current branch defines the application direction, storage layout and guarded Win32 window-host scaffold; successful build, Xbox deployment, and guest boot have not yet been verified. In the Xbox UWP app partition, the Win32 host currently reports `ERROR_NOT_SUPPORTED` rather than claiming that a desktop window was created.

## Project plan

See [docs/VMWARE-XBOX-PLAN.md](docs/VMWARE-XBOX-PLAN.md) and [docs/OS-PROFILES.md](docs/OS-PROFILES.md).

The original OnyxOS `main` branch remains preserved. Legacy QEMU source is not yet removed from this branch; it must be audited and detached from the build before claiming a clean VMware-only executable.
