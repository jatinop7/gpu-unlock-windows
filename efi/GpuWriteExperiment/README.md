# Experimental PCI configuration transaction

This is a separate, opt-in UEFI application. It is separate from the PCI
observation probe. Its profile identifies the Acer Nitro V 15 RTX 4050 (`10DE:28A1`,
subsystem `1025:1758`), but has no write actions and is denied, so the
application exits without writing anything.

The transaction framework can apply a reviewed list of 32-bit PCI extended
configuration values. It requires one unique vendor/device/subsystem ID match
(and can optionally pin an exact BDF), exact preflight values for every action, an operator keypress, and
readback verification. It snapshots old values and attempts reverse-order
rollback if a write or readback fails. If rollback cannot be verified, it
intentionally does not return to the boot flow. A profile must also declare an
independent recovery method and that recovery has been tested; otherwise the
app refuses to run.

The transaction app is a framework; its checked-in RTX 4050 profile contains
identity only. It is not the Ada unlock backend. The benchmark is documented in
`docs/HARDWARE-RESULTS.md`.

A hard hardware lock can prevent rollback code from running, so a profile must
include an independent recovery method tested on the exact laptop.

Before enabling any profile, its author must establish that each register is
safe to write as a full dword, that the expected-value checks are correct, that
rollback is valid for that register's semantics, and that the machine has an
independent recovery path, then verify that recovery on that exact laptop. PCI
extended config writes can still hang or reset a device; an EFI application
cannot guarantee Windows will initialize it after handoff. The app does not
write VBIOS, SPI flash, or persistent firmware.

Do not add a profile by guessing.

## Build

From an EDK II workspace where this repository is under `gpu-unlock-windows`:

    build -p OvmfPkg/OvmfPkgX64.dsc -m gpu-unlock-windows/efi/GpuWriteExperiment/GpuWriteExperiment.inf -a X64 -t VS2022 -b DEBUG

This builds an EFI application only. With the checked-in profile, launching it
prints `WRITE_DISABLED` and performs no hardware writes.
