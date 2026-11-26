# VMware-UWP-Xbox

## Project direction

This branch adapts the OnyxOS scaffold into a VMware Workstation 12-inspired Xbox UWP shell. The first milestone is the desktop-style application window and its VM library/console experience. It is **not** a port of VMware Workstation 12's proprietary hypervisor or binaries.

## Target development environment

- Visual Studio 2026
- C++ / MSVC
- x64 UWP package (APPX), subject to Xbox Dev Mode API support
- Windows SDK 10.0.26100.0 where supported by the installed toolchain
- Win32-style window/message handling where the Xbox UWP contract permits it
- Direct3D 11 for custom rendering if needed

## Window and input design

1. Main VMware-style application surface: full-screen, with menus, toolbar, Library, VM details and settings.
2. VM Console surface: a separate top-level Win32 window only if the Xbox/UWP app model supports it. Otherwise host the console as a second in-process view/surface. Do not assume desktop multi-window APIs are available on Xbox.
3. Console input: physical keyboard input is routed to the selected console. A virtual mouse cursor is moved with the Xbox controller; A/RT click and B/LT right-click are configurable defaults.
4. Keyboard overlay: provide an on-screen keyboard fallback when no physical keyboard is connected.
5. Focus/capture: provide explicit capture/release controls so controller navigation can return to the main library.

## VM profile scope

- Initial guest architecture target: x86 (32-bit) only.
- x64 guests are out of scope for the first milestone.
- 32-bit guest support does not itself remove the need for hardware virtualization; a compatible software execution engine would still be required.
- VMware Workstation 12 UI inspiration does not imply that VMware's original host executable, drivers, hypervisor, or proprietary runtime can run in UWP.

## App-private storage

Create directories on first launch under the package's LocalState folder. Do not recreate or modify the user's Windows Documents, Desktop, Downloads, or other personal folders.

```text
LocalState/
└── VMware/
    ├── Config/
    ├── Virtual Machines/
    ├── ISO/
    ├── Disks/
    ├── Snapshots/
    ├── Logs/
    └── Cache/
```

Use the package's app-local storage API/path to resolve LocalState at runtime; never hard-code a user profile path. Create directories idempotently and handle access/errors.

## Implementation milestones

- [ ] Rename product-facing branding to VMware-UWP-Xbox while preserving package identity unless a deliberate identity migration is needed.
- [ ] Replace the OnyxOS launcher concept with the VMware-style main window.
- [ ] Add Library, toolbar, menus, VM summary and settings screens.
- [ ] Add LocalState directory initialization and configuration persistence.
- [ ] Add console view/window with Direct3D 11 rendering surface.
- [ ] Add physical keyboard routing and controller-driven virtual mouse.
- [ ] Validate whether a second top-level Win32 window is supported by Xbox UWP; fall back to an in-process console surface if not.
- [ ] Decide and integrate a legally redistributable x86 software execution engine, if actual guest execution is later required.
- [ ] Build, package, deploy and test on Xbox Dev Mode.

## Build status and constraints

This document is a design/implementation plan, not evidence of a successful build or working VM. Xbox UWP is sandboxed. It does not grant unrestricted desktop Win32, arbitrary child processes, kernel drivers, VT-x access, or guaranteed multiple top-level windows. Verify each API against the Xbox target and installed SDK before relying on it.

The existing OnyxOS/QEMU code is retained on this branch for now. Remove or replace it only after the new shell builds and the desired replacement runtime is selected.
