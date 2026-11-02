# Hardware test results

This file records results by exact laptop and GPU configuration. New GPU tests can be added as separate entries.

The RTX 4050 entry is the recorded Ada / RTX 40 result and starting hardware target. RTX 20, 30, and 50 are planned future targets; see the [RTX generation scope](../README.md#rtx-generation-scope).

## Acer Nitro V 15 ANV15-41 · RTX 4050 Laptop GPU 6 GB

| Check | Result |
|---|---|
| GPU | NVIDIA GeForce RTX 4050 Laptop GPU, AD107M |
| VRAM | 6141 MiB reported (6 GB class) |
| PCI vendor/device ID | `10DE:28A1` |
| PCI subsystem vendor/device ID | `1025:1758` (Acer) |
| VBIOS | `95.07.43.00.ae` |
| Windows boot after experiment | Successful |
| Benchmark | CUDA 12.6 / cuBLAS 12.6, 4096 × 4096 GEMM |
| FP16 input, FP32 accumulation/output | 41.57 → 78.87 TFLOPS |
| BF16 input, FP32 accumulation/output | 41.51 → 78.67 TFLOPS |
| FP16 change | +37.30 TFLOPS (+89.7%, 1.90×) |
| BF16 change | +37.16 TFLOPS (+89.5%, 1.90×) |
| Benchmark procedure | 2-second warmup per precision, 30 samples, 3 trials; median result |

The benchmark configuration and a small CUDA/cuBLAS reproducer are in [CUBLAS-BENCHMARK.md](../tools/CUBLAS-BENCHMARK.md).
The reported baseline and candidate medians are also listed in [benchmark-raw.csv](../artifacts/benchmark-raw.csv), and the machine snapshot is in [system-inventory.json](../artifacts/system-inventory.json).

## Recorded EFI transaction

The Acer profile uses the two masked PCI configuration writes recorded in the successful EFI run:

| Action | Offset | Before | Mask | Written | Readback |
|---:|---:|---:|---:|---:|---:|
| 1 | `0x100` | `0x00000000` | `0x0000000F` | `0x00000005` | `0x00000005` |
| 2 | `0x104` | `0x00000010` | `0x0000000F` | `0x0000001A` | `0x0000001A` |

The EFI report recorded two hardware writes, successful readback verification, and successful Windows handoff.
See the [complete EFI transaction log](EFI-TRANSACTIONS.md), [PCI register map](PCI-REGISTER-MAP.md), and [failure analysis](FAILURE-ANALYSIS.md) for the action details and recovery record.

## Add another GPU result…

Add each tested device as a separate entry with its laptop or desktop model, GPU model and VRAM, PCI device/subsystem IDs, BIOS and VBIOS versions, Windows and driver versions, benchmark configuration, before/after measurements, and Windows boot outcome.
