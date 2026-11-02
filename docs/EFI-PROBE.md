# EFI probe workflow

The first pre-boot component is a UEFI observation application. It collects PCI identity and location for display controllers and PCI bridges, PCI command/status values, bridge bus ranges, BAR resource descriptors, Secure Boot state, and same-handle GOP presence. It performs no PCI, MMIO, PROM, file, or boot-variable writes.

It does not read GPU MMIO registers, PROM/VBIOS bytes, or firmware images. It does not write PCI configuration, map DMA, change UEFI variables, register boot entries, or modify a disk. Same-handle GOP presence is not a complete display-ownership result.

## Build and run

See the external EDK II build instructions in the [probe directory](../efi/GpuProbe/README.md). Use a separate FAT32 USB device and the one-time firmware boot menu. Do not stage the probe on the Windows system EFI partition. The output is text on the console; capture only the lines from GPU_UNLOCK_DIAG through END.

Parse the capture offline:

    python -I tools/parse_efi_capture.py capture.txt -o capture.json

The parser rejects malformed and duplicate records. It produces a SHA-256 digest of the text input and always marks the result not assessed.

## Limits

This is an initial snapshot, not a write authorization. The probe does not establish complete PCI ancestry or protocol ownership, and a single run does not establish cold-start stability. Do not derive a supported profile from an inventory result alone.
