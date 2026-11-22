# OnyxOS

**Xbox UWP virtual machine host powered by QEMU**

OnyxOS is an experimental x64 Xbox application intended to boot an unmodified Windows 7 x64 guest through QEMU. The target host UI is native C++ **CoreWindow**, with Direct3D 11/HLSL guest display rendering and Xbox controller, mouse, and keyboard input.

> **Development status:** this branch adds the Windows 7 QCOW2 profile, runtime image download/import helpers, QEMU argument configuration, and a native CoreWindow Direct3D renderer. The existing application entry point is still the legacy XAML host; the full XAML removal, CoreWindow app lifecycle, automatic VM launch, on-screen keyboard, and end-to-end Xbox boot test are not complete yet. No successful Windows 7 guest boot is claimed.

## Windows 7 x64 guest

The default profile is `windows7`:

| Setting | Value |
| --- | --- |
| Guest | Unmodified Windows 7 x64 image |
| Disk | QCOW2 |
| CPU model | `max` |
| vCPU count | 2 |
| Guest memory | 2560 MB (2.5 GB) |
| Machine | PC / SeaBIOS |
| Disk device | IDE |
| Graphics adapter | Standard VGA |
| Audio | Disabled |
| Acceleration | QEMU TCG with multi-threading |
| Display target | CoreWindow + Direct3D 11 + HLSL |

The memory value is a guest allocation target, not a guarantee that the Xbox can reserve it alongside the app, QEMU, and graphics buffers. The profile must be tested on the actual console.

## Windows 7 disk: two ways

1. **Download in the app:** the app can download the QCOW2 file from the Archive.org URL supplied for this project into its persistent app storage.
2. **Use a local file:** the app can import a user-selected QCOW2 file into the same persistent storage.

Both paths validate the QCOW2 magic header before returning the file to the VM layer. The image is not checked into Git and is not downloaded by GitHub Actions; this avoids storing a multi-gigabyte guest OS disk in CI artifacts. Users are responsible for the image's provenance and licensing.

## Graphics

- `src/graphics/CoreWindowD3DRenderer.*` — native DXGI swap chain targeting CoreWindow.
- `src/graphics/QemuD3DRenderer.*` — existing legacy XAML SwapChainPanel renderer.
- `graphics/shaders/onyxos.hlsl` — full-screen triangle HLSL shader.
- The full-screen triangle uses normalized UV coordinates and a full-surface viewport to address the prior half-screen/cropped guest display symptom.

The CoreWindow renderer is compiled into the project but is not yet connected to the app lifecycle or QEMU frame callback. Guest GPU passthrough/3D acceleration is not provided; HLSL renders the QEMU framebuffer on the Xbox host.

## Input target

- Xbox controller: left stick moves the guest pointer; A/B are mouse buttons; D-pad sends arrow keys.
- Physical USB mouse and keyboard are passed through the QEMU input bridge.
- On-screen keyboard is planned for controller-only text entry.

Input mapping and virtual keyboard UI still need to be connected to the native CoreWindow app.

## Build

- `.github/workflows/build-uwp.yml` builds the x64 UWP package on Windows.
- `Build.ps1` stages the QEMU UWP runtime, firmware, HLSL asset, and APPX.
- The workflow artifact contains the app package and certificate, not a Windows guest disk.

## Important technical limitations

- QEMU TCG is software emulation; performance depends on the Xbox CPU and the UWP runtime.
- `-cpu max` selects QEMU's maximum compatible CPU model; it does not unlock extra physical CPU cores.
- The `codeGeneration` capability is declared for TCG JIT, but the console/package policy must permit it at runtime.
- A 2.5 GB guest allocation may fail if the Xbox app memory partition cannot provide enough contiguous/available memory.
- CoreWindow app lifecycle, VM auto-start, image picker/download UI, physical input routing, and guest boot must be completed and tested on Xbox before this branch is considered usable.

## Repository layout

```text
config/                  Guest profile metadata
docs/                    Architecture and implementation notes
graphics/shaders/        HLSL and GLSL sources
src/downloads/            Guest disk download/import helpers
src/graphics/             XAML and CoreWindow Direct3D renderers
src/vm/                   QEMU configuration and host/input bridges
.github/workflows/        GitHub Actions build
```

## License

OnyxOS is licensed under GPL-3.0. Third-party QEMU components retain their own licenses. Windows is proprietary software; this repository does not redistribute a Windows license or guarantee that a third-party disk image is authorized for redistribution.
