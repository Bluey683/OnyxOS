# Native CoreWindow guest display

The Xbox host uses `CoreApplication` / `IFrameworkView` and a native
`CoreWindow` with a Direct3D 11 composition-independent swap chain. No XAML
pages, generated XAML headers, or `SwapChainPanel` controls are part of this
branch.

## Current state

- CoreWindow lifecycle and visibility handling: implemented in `src/App.cpp`.
- Direct3D 11 device, swap chain, render target and clear/present loop: implemented.
- Keyboard and pointer event hooks: implemented as host input diagnostics.
- QEMU framebuffer callback to the CoreWindow renderer: not connected yet.
- QEMU VM start/stop, guest input routing, audio and controller polling: not connected
  to the CoreWindow event loop yet.

The current renderer clears the host surface; it does not yet display a guest
operating system. A successful package build and Xbox runtime test are still
required before claiming device compatibility.
