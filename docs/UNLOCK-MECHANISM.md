# Unlock mechanism hypothesis

## Observed result

On the Acer Nitro V 15 ANV15-41 with an RTX 4050 Laptop GPU, the recorded 4096 × 4096 cuBLAS GEMM medians changed as follows after the EFI transaction:

| Mode | Before | After | Change |
|---|---:|---:|---:|
| FP16 input, FP32 accumulation/output | 41.57 TFLOPS | 78.87 TFLOPS | 1.90× |
| BF16 input, FP32 accumulation/output | 41.51 TFLOPS | 78.67 TFLOPS | 1.90× |

The EFI log records two PCI configuration writes, successful readbacks, and a successful Windows handoff. The full values are in [EFI transactions](EFI-TRANSACTIONS.md); the benchmark setup is in [reproducibility](REPRODUCIBILITY.md).

## Working hypothesis

The experiment tested whether changing firmware-visible GPU configuration before Windows initializes the NVIDIA driver changes the GPU's operating behavior under compute load. The measured throughput increase is consistent with a change in the device or driver initialization state. The available records do not identify a specific clock, voltage, power-limit, or Tensor Core control changed by the writes.

The addresses require particular care. Offset `0x100` is where PCI extended-configuration capability enumeration begins in the Linux PCI implementation. Its first dword is interpreted as an extended-capability header: the low 16 bits are the capability ID, the next 4 bits are the version, and the upper bits hold the next-capability pointer. The recorded write changes that dword from `0x00000000` to `0x00000005`; under that standard interpretation, the resulting ID is `0x0005`, Root Complex Link Declaration. The supplied records do not include an NVIDIA device document that identifies this as a performance control. See the [register map](PCI-REGISTER-MAP.md) and the Linux PCI definitions it cites.

The defensible conclusion is that the transaction and the faster benchmark result were observed in the same run. The mechanism remains a hypothesis until the exact GPU capability chain, vendor documentation, and before/after power and clock telemetry identify the changed operating state.

## Evidence and limits

Evidence currently recorded:

- Exact GPU and subsystem identity: `10DE:28A1`, `1025:1758`.
- Two 32-bit PCI configuration writes at offsets `0x100` and `0x104`, each with a low-nibble write mask.
- Readback values matching both requested values.
- The reported FP16 and BF16 benchmark medians before and after the experiment.
- Successful EFI status, Windows handoff, and subsequent Windows boot.

The transaction report does not identify a documented NVIDIA performance register at either offset. The benchmark record does not include power draw, power limit, clock, voltage, temperature, or throttle-reason samples for the baseline and candidate runs. Those observations are listed in [power analysis](POWER-ANALYSIS.md).

## References

- Linux PCI configuration-space sizes and extended-capability header fields: [`include/uapi/linux/pci_regs.h`](https://github.com/torvalds/linux/blob/master/include/uapi/linux/pci_regs.h).
- Linux extended-capability enumeration, which starts scanning at `PCI_CFG_SPACE_SIZE` and stops on an all-zero header: [`drivers/pci/pci.h`](https://github.com/torvalds/linux/blob/master/drivers/pci/pci.h).
- NVIDIA cuBLAS 12.6 `cublasGemmEx` API and compute types: [cuBLAS 12.6 documentation](https://docs.nvidia.com/cuda/archive/12.6.2/cublas/index.html).
