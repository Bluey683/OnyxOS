# QEMU-UWP-Host integration

OnyxOS uses the upstream [QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host)
runtime as its embedded QEMU implementation. The runtime is fetched by
`scripts/Fetch-QemuRuntime.ps1` from immutable upstream commit
`53d6af6314f535c4c71c4be269777a710063327b` (upstream branch: `x86-64`).

## What the fetch step stages

- `Dependencies/Qemu/qemu-system-x86_64.dll`: upstream UWP QEMU host DLL.
- `Dependencies/Runtime/*.dll`: matching runtime dependency set.
- `qemu/`: upstream firmware, BIOS, UEFI images, descriptors and keymaps.
- The source project license is retained in `Dependencies/QEMU-UWP-Host-LICENSE`.

Run `scripts/Fetch-QemuRuntime.ps1` directly or let `Build.ps1` run it.
The fetch script checks the pinned DLL size, runtime dependency count, BIOS and
x86_64 UEFI firmware before packaging. Do not mix the DLL with runtime files
from another QEMU build.

## ABI contract

The existing `src/vm/QemuHostAdapter.*` is OnyxOS's native adapter for the
upstream exported C ABI. It validates API major version 1 and required
initialization, main-loop, input, shutdown and video callback exports. The
upstream API specification is copied to `docs/QEMU-UWP-HOST-API.md`.

## Current integration boundary

The upstream runtime and its dependencies are staged for the x64 APPX. The
CoreWindow branch currently has a Direct3D 11 host surface and a separate
QEMU adapter, but the adapter is not yet owned by the CoreWindow lifecycle,
and the guest frame callback is not yet uploaded to the CoreWindow swap chain.
Audio output and end-to-end VM start/stop are also not yet verified. Therefore
this is a pinned runtime integration foundation, not a claim that a guest OS
already boots on Xbox.
