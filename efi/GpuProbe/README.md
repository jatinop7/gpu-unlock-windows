# EFI PCI probe

This UEFI application enumerates PCI display controllers and PCI-to-PCI bridges through the standard PCI I/O protocol. It reports PCI identity and location, command/status values, bridge bus ranges, BAR resource descriptors, Secure Boot state, and whether a GOP protocol is present on the same PCI handle.

The probe contains no PCI configuration write call, MMIO access, PROM/VBIOS read, filesystem write, boot-variable write, or unlock operation. A same-handle GOP result is only a clue; it does not establish the complete console ownership graph. The separate experimental transaction app is under `../GpuWriteExperiment`; it contains the enabled Acer RTX 4050 two-action profile.

## Build

The EDK II source tree is an external build dependency and is not included here. Place this project inside an EDK II workspace under the folder name gpu-unlock-windows, ensure the workspace includes MdePkg, and use the OVMF X64 platform description to resolve the application libraries:

    build -p OvmfPkg/OvmfPkgX64.dsc -m gpu-unlock-windows/efi/GpuProbe/GpuReadOnlyProbe.inf -a X64 -t VS2022 -b DEBUG

This builds only the EFI application; it does not build or flash OVMF firmware. The toolchain tag depends on the local setup.

## Run and capture

Copy only the resulting EFI application to a separate FAT32 USB device and launch it from the firmware boot menu. Do not run it from the Windows system EFI partition. It prints a text capture to the console and waits for a key. Photograph or transcribe the output; review it for personal details before sharing.

This probe is an initial inventory, not a compatibility result. It does not inspect GPU MMIO registers, option-ROM bytes, complete PCI ancestry, or driver ownership beyond the same-handle GOP check.
