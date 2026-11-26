# VMware-UWP-Xbox

A Visual Studio 2026 / C++ project direction for an Xbox Dev Mode UWP application inspired by the VMware Workstation 12 desktop interface.

**Current milestone: application shell and window/input design.** This repository does not claim to port VMware Workstation 12's proprietary hypervisor or to boot guest operating systems.

## Initial target

- Xbox One / Xbox Series X|S Dev Mode, subject to API and package validation.
- C++ and MSVC, x64 app package.
- Visual Studio 2026 and a compatible Windows SDK (10.0.26100.0 is the intended baseline).
- VMware Workstation 12-inspired full-screen main interface.
- Initial guest architecture target: x86 (32-bit); actual execution engine is a separate future decision.
- No XAML requirement for the planned custom Win32/Direct3D interface.

## Planned experience

- Main window: VMware-style menus, toolbar, Library, VM details and settings.
- Console: a second Win32 top-level window if Xbox UWP supports it; otherwise a second in-process view/surface.
- Input: native keyboard routing, controller-driven virtual mouse, and optional on-screen keyboard.
- Storage: app-private `LocalState/VMware/` directories for configuration, VM files, ISO images, disks, snapshots, logs and cache.
- No creation or modification of the user's Windows Documents/Desktop/Downloads folders.

## Important technical constraints

Xbox UWP is not unrestricted desktop Windows. A Win32 API being available in desktop Windows does not guarantee it is available to an Xbox UWP app. In particular, separate top-level windows, child processes, drivers, unrestricted file access and VT-x must not be assumed.

A 32-bit guest target does not automatically eliminate the need for hardware virtualization. Actual guest execution requires a compatible software execution engine or supported hardware virtualization path. The original VMware Workstation 12 host application and its drivers cannot simply be copied into an APPX.

## Project documentation

- [VMware Xbox architecture and implementation plan](docs/VMWARE-XBOX-PLAN.md)
- [Original OnyxOS history](https://github.com/Bluey683/OnyxOS/tree/main)

## Branching and preservation

This work is being staged on the `VMware-UWP-Xbox` branch. The original `main` branch is preserved. Existing QEMU scaffold files are retained until a replacement execution engine is selected and the new application shell is validated.
