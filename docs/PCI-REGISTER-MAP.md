# PCI register map

This map records the two 32-bit PCI configuration writes in the Acer RTX 4050 profile. It separates the values observed in the EFI report from the standard PCI meanings that can be assigned to those addresses.

## Transaction values

| Offset | Width | Reported name | Before | Write mask | Requested value | Readback | Meaning established by available references |
|---:|---:|---|---:|---:|---:|---:|---|
| `0x100` | 32-bit | PCI extended configuration | `0x00000000` | `0x0000000F` | `0x00000005` | `0x00000005` | Start of extended-capability enumeration. If interpreted as a standard capability header, low 16 bits change from ID `0` to ID `5` (`Root Complex Link Declaration`). No NVIDIA performance-control meaning is documented here. |
| `0x104` | 32-bit | PCI extended configuration | `0x00000010` | `0x0000000F` | `0x0000001A` | `0x0000001A` | Dword immediately following offset `0x100`. Its register name and semantics depend on the capability/device definition. No NVIDIA reference identifying it as a performance control is available. |

The writer preserves bits outside each mask. Its readback mask is `0xFFFFFFFF`, so it verifies the complete dword against the value it wrote. The exact transaction and bit-change counts are in [EFI transactions](EFI-TRANSACTIONS.md).

## Standard PCI interpretation

PCIe extended configuration space extends the configuration region from 256 bytes to 4096 bytes. Linux begins extended-capability enumeration at byte offset 256 (`0x100`). Its PCI definitions decode an extended-capability header as:

- bits 15:0: capability ID;
- bits 19:16: capability version;
- bits 31:20: next-capability pointer.

The same definitions assign capability ID `0x0005` to Root Complex Link Declaration. Linux's enumeration code treats an all-zero header as the end of the list. Consequently, the recorded `0x100` values (`0` before, `5` after) do not identify a documented NVIDIA clock, voltage, or power-limit register; read literally under the standard header layout, the write changes the capability ID field. The exact hardware behavior and the association with the benchmark result are not established by the report alone.

## References

- Linux PCI configuration-space sizes and extended-capability header definitions: [`include/uapi/linux/pci_regs.h`](https://github.com/torvalds/linux/blob/master/include/uapi/linux/pci_regs.h).
- Linux PCI extended-capability enumeration and zero-header handling: [`drivers/pci/pci.h`](https://github.com/torvalds/linux/blob/master/drivers/pci/pci.h).
- NVIDIA's public [nvidia-smi documentation](https://docs.nvidia.com/deploy/nvidia-smi/) describes power, clocks, temperature, and throttle telemetry. It does not document offsets `0x100` or `0x104` as RTX 4050 performance controls.
