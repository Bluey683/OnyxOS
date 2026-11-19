# VM Devices: Audio, Input and Network

## Audio
VM command generation selects QEMU's `xaudio2` audio backend and an Intel HDA
guest controller with an HDA duplex codec:

- `-audiodev xaudio2,id=audio0`
- `-device intel-hda`
- `-device hda-duplex,audiodev=audio0`

This requires the packaged QEMU UWP build to include its XAudio2 backend. The
actual Xbox audio output still needs an end-to-end run test on the target console.

## Keyboard and mouse
`QemuInputBridge` maps CoreWindow key events to SDL-compatible QEMU key numbers.
Pointer motion is sent as relative movement when the host DLL exports
`qemu_host_send_pointer_rel`, with absolute-coordinate fallback. Mouse buttons
are forwarded using the QEMU host input API (button IDs: 0 left, 1 middle, 2 right).

## Xbox controller
The bridge polls `Windows.Gaming.Input.Gamepad`:
- Left stick moves the guest pointer.
- D-pad sends arrow keys.
- A sends Enter; B sends Escape.
- X/Y hold left/right mouse buttons.
- A deadzone filters stick drift.

The bridge releases held keys/buttons when the controller disconnects or the app
stops. Controller-to-key mapping is a starter layout and can be customized later.

## Internet
VMs use QEMU user-mode networking (NAT) with an emulated e1000e network adapter.
This avoids requiring a bridged network interface or raw socket access. Windows
and Linux guests may still need their network driver/configuration to be active.
The APPX already declares `internetClient` and
`privateNetworkClientServer` capabilities.

## Integration boundary
These changes add device configuration, host-API forwarding and UWP input event
handlers. They do not yet make a guest boot: the VM lifecycle, argument handoff,
QEMU main-loop thread, video callback and audio runtime still need to be connected
and validated together. Input calls only work while the host DLL has an active
initialized guest session. No Xbox console build or sound/input/network test has
been run in this change.
