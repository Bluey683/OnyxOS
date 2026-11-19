# XAML guest display for the six OS entries

`GuestPage.xaml` is the shared guest-console layout for the six non-launcher
entries in the single APPX: Windows 10, Windows 11, Debian, Ubuntu, Arch Linux
and experimental macOS. It contains a `SwapChainPanel` intended for the QEMU
framebuffer, status overlays and a stop control.

## Renderer policy

- Xbox / UWP: HLSL + Direct3D 11 is the primary renderer.
- GLSL: optional alternative backend for environments that provide a compatible
  OpenGL implementation. GLSL 330 is not assumed to be available on Xbox.
- Both shader paths sample the QEMU guest framebuffer; they do not provide GPU
  passthrough or guest 3D acceleration.

## Important integration status

The XAML page is a UI source scaffold. The current app entry point still uses
CoreWindow `IFrameworkView`, and CMake does not run the UWP XAML compiler or
generate the `GuestPage.g.h/.g.cpp` files. The page therefore is not yet shown
by the six packaged executables.

The upstream QEMU DLL supports video callback registration. The OnyxOS adapter
still needs to register that callback, copy the callback pixels safely, create
a D3D11 texture/swap chain, and present frames to `SwapChainPanel`. Until that
bridge and the XAML application host are built and tested, the guest display
must be treated as pending, not working.
