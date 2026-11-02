# EFI transactions

This page records the successful Acer Nitro V 15 RTX 4050 transaction report supplied for the reference run. Values below are reproduced from that report.

## Target and preconditions

| Field | Value |
|---|---|
| GPU | NVIDIA GeForce RTX 4050 Laptop GPU |
| PCI vendor/device | `10DE:28A1` |
| PCI subsystem vendor/device | `1025:1758` |
| Architecture | Ada Lovelace |
| EFI module/version | `GpuWriteExperiment.efi` / `0.1.0` |
| Reported build ID | `EFI-RTX4050-2026-001` |
| Profile | `acer-nitro-v15-rtx4050` |

The profile requires the exact PCI and subsystem IDs, one matching device, expected values at both offsets, aligned extended-configuration offsets, valid masks, and operator confirmation with `Y`. If a write or readback fails, the current source attempts reverse-order masked rollback and verifies the restored masked bits. The successful report below did not enter the rollback path.

## Recorded transaction log

```text
============================================================
       NVIDIA RTX 4050 - EFI WRITE TRANSACTION REPORT
============================================================

GPU             : NVIDIA GeForce RTX 4050 Laptop GPU
Vendor ID       : 10DE
Device ID       : 28A1
Subsystem       : 1025:1758
Architecture    : Ada Lovelace

EFI Module      : GpuWriteExperiment.efi
Build ID        : EFI-RTX4050-2026-001
Profile         : acer-nitro-v15-rtx4050
Execution Mode  : EXPERIMENTAL

------------------------------------------------------------
TRANSACTION 001
------------------------------------------------------------

Operation       : WRITE
Address Space   : PCI Configuration Space
Offset          : 0x100
Width           : 32-bit
Before Value    : 0x00000000
Write Mask      : 0x0000000F
Written Value   : 0x00000005
Readback Value  : 0x00000005
Bits Changed    : 2
Verification    : PASS

------------------------------------------------------------
TRANSACTION 002
------------------------------------------------------------

Operation       : WRITE
Address Space   : PCI Configuration Space
Offset          : 0x104
Width           : 32-bit
Before Value    : 0x00000010
Write Mask      : 0x0000000F
Written Value   : 0x0000001A
Readback Value  : 0x0000001A
Bits Changed    : 2
Verification    : PASS

------------------------------------------------------------
FINAL RESULT
------------------------------------------------------------

Hardware Writes : 2
Readback Checks : 2
Verification    : PASS
EFI Status      : SUCCESS
Boot Handoff    : SUCCESS
```

## Rollback outcome

The recorded run completed successfully, so no rollback was triggered. The report contains no forced-failure run or rollback readback values. The source implements rollback for a failed transaction; this successful log is not a rollback test.

The transaction values are listed with their available standard PCI interpretations in the [PCI register map](PCI-REGISTER-MAP.md). The current records do not provide NVIDIA documentation naming these offsets as performance controls.
