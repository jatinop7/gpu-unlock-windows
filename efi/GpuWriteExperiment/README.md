# Experimental PCI configuration transaction

This is a separate, opt-in UEFI application. It is separate from the PCI
observation probe. Its enabled profile targets the Acer Nitro V 15 ANV15-41
RTX 4050 (`10DE:28A1`, subsystem `1025:1758`) and includes the two actions in
the recorded EFI transaction:

| PCI config offset | Expected dword | Write mask | Desired dword | Readback mask |
|---:|---:|---:|---:|---:|
| `0x100` | `0x00000000` | `0x0000000F` | `0x00000005` | `0xFFFFFFFF` |
| `0x104` | `0x00000010` | `0x0000000F` | `0x0000001A` | `0xFFFFFFFF` |

The app requires one unique vendor/device/subsystem ID match, exact preflight
values for every action, and an operator `Y` confirmation. It changes only the
low nibble selected by each write mask, verifies readback, and attempts
reverse-order masked rollback if a write or readback fails. If rollback cannot
be verified, it stops in firmware instead of handing off to Windows. The
profile also records the independent recovery route: power off, remove the EFI
USB, and boot Windows Boot Manager.

The profile is scoped to the Acer PCI and subsystem IDs. Its recorded write
sequence and benchmark are documented in `docs/HARDWARE-RESULTS.md`.

A hard hardware lock can prevent rollback code from running, so a profile must
include an independent recovery method tested on the exact laptop.

For future profiles, establish the meaning of each register, validate the
expected values and rollback behavior, and test an independent recovery path on
the exact machine before enabling it. PCI extended-configuration writes can
still hang or reset a device; an EFI application cannot guarantee Windows will
initialize it after handoff. The app does not write VBIOS, SPI flash, or
persistent firmware.

Do not add a profile by guessing.

## Build

From an EDK II workspace where this repository is under `gpu-unlock-windows`:

    build -p OvmfPkg/OvmfPkgX64.dsc -m gpu-unlock-windows/efi/GpuWriteExperiment/GpuWriteExperiment.inf -a X64 -t VS2022 -b DEBUG

This builds the EFI application. At launch, it prints the target and both
masked write values, then waits for `Y`; any other key cancels.
