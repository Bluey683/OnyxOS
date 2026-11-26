# VMware-UWP-Xbox

## Project direction

Adapt the OnyxOS scaffold into a VMware Workstation 12-inspired Xbox UWP shell. The first milestone is the full-screen desktop-style application window and its VM library/console experience. This is not a port of VMware Workstation 12's proprietary hypervisor or binaries.

## Target development environment

- Visual Studio 2026
- C++ / MSVC
- x64 UWP package (subject to Xbox Dev Mode API support)
- Compatible Windows SDK; 10.0.26100.0 is the intended baseline
- Win32-style window/message handling where the Xbox UWP contract permits it
- Direct3D 11 for custom rendering if needed
- No XAML in the planned custom interface

## Only guest profile in the initial scope

- Windows x86 (32-bit) only.
- No Linux, macOS, Windows x64, or additional guest profiles.
- No Windows ISO or installer is bundled. The user supplies compatible installation media if needed and is responsible for licensing.
- A 32-bit guest does not itself remove the need for a supported virtualization or software-emulation engine.

## Window and input design

1. Main VMware-style application surface: full-screen, with menus, toolbar, Library, VM details and settings.
2. VM Console: a separate top-level Win32 window only if the Xbox/UWP app model supports it. Otherwise host the console as a second in-process view/surface.
3. Console input: physical keyboard input is routed to the selected console. A virtual mouse cursor is moved with the Xbox controller; A/RT and B/LT are configurable click defaults.
4. Keyboard overlay: provide an on-screen keyboard fallback when no physical keyboard is connected.
5. Focus/capture: provide explicit capture/release controls so controller navigation can return to the main library.

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

- [x] Rename product-facing documentation to VMware-UWP-Xbox while preserving package identity.
- [x] Define Windows x86 as the sole initial guest profile.
- [x] Add app-private LocalState directory initialization.
- [ ] Replace the OnyxOS launcher concept with the VMware-style main window.
- [ ] Add Library, toolbar, menus, VM summary and settings screens.
- [ ] Add console view/window with Direct3D 11 rendering surface.
- [ ] Add physical keyboard routing and controller-driven virtual mouse.
- [ ] Validate whether a second top-level Win32 window is supported by Xbox UWP; fall back to an in-process console surface if not.
- [ ] Audit and remove legacy QEMU targets/dependencies from the VMware build.
- [ ] Select and integrate a compatible guest execution engine, if actual guest execution is required.
- [ ] Build, package, deploy and test on Xbox Dev Mode.

## Build status and constraints

This is a design and partial implementation, not evidence of a successful build or working VM. Xbox UWP is sandboxed. It does not grant unrestricted desktop Win32, arbitrary child processes, kernel drivers, VT-x access, or guaranteed multiple top-level windows. Verify each API against the Xbox target and installed SDK before relying on it.

The original OnyxOS `main` branch is preserved. Legacy QEMU source remains in this branch until the build files and runtime dependencies can be safely audited and removed.
