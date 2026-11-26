# VMware-UWP-Xbox: guest profile

The initial VMware-UWP-Xbox release exposes exactly one guest profile:

| Display name | Guest architecture | State |
|---|---|---|
| Windows 32-bit | x86 (32-bit) | Initial profile |

There are no Linux, macOS, Windows x64, or additional guest profiles in this initial scope.

## Installation media

The project does not bundle Windows installation media and does not generate an installer. If a fresh guest installation is required, the user supplies their own compatible Windows x86 installation media and confirms they have the appropriate license. The app stores user-selected media only in its app-private storage after the user chooses it.

## App-private storage

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

These folders are created inside the package's LocalState. The application must not create or modify the user's Windows Documents, Desktop, Downloads, or other personal folders.

## Important execution boundary

This profile is metadata for the VMware-style Xbox application. It does not make VMware Workstation 12's desktop hypervisor or drivers compatible with Xbox UWP. A 32-bit guest does not automatically remove the need for a supported virtualization or software-emulation engine. Guest boot must remain disabled until a compatible engine is integrated and tested on Xbox Dev Mode.
