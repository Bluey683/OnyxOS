# VM Engine

The first VM engine layer provides validated VM configuration and QEMU argument
generation for the OnyxOS x86_64 guest profiles.

## Current capabilities

- Configuration model: guest ID/name, OS family, ISO, VHDX, machine type,
  memory, vCPU count and UEFI preference.
- Input validation before a VM can be handed to the runtime.
- QEMU argument generation for a q35 guest with a VHDX disk and installer ISO.
- The model is shared by the seven packaged app entries.

## Storage layout

```
LocalState/
  ISO/
  VHDX/
  Configurations/
  Logs/
```

## Important implementation boundary

This is the VM configuration layer, **not yet a running hypervisor**. It does
not launch a process, load the QEMU UWP host DLL, create VHDX files, locate UEFI
firmware, render the guest framebuffer, or connect audio/input/network devices.
The Xbox UWP runtime must call the supported QEMU host API in-process; ordinary
desktop process creation is not an acceptable substitute in the AppContainer.

The generated arguments are a starting point for that adapter. UEFI firmware
arguments must be appended only after the exact firmware files and licensing/
redistribution terms are established. Windows 11 TPM/Secure Boot support and
macOS compatibility remain unimplemented.
