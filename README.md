# Windows GPU Unlock

GPU performance and pre-boot research for Windows systems with NVIDIA RTX GPUs. The first machine target is the Acer Nitro V 15 ANV15-41 with its RTX 4050 Laptop GPU. Other GPU models are future work.

> **Project status:** The RTX 4050 EFI unlock backend is not implemented in this repository yet. The checked-in writer is disabled and cannot reproduce the recorded benchmark run.

## Start here

| Goal | Guide |
|---|---|
| See the RTX generation scope | [RTX generation scope](#rtx-generation-scope) |
| Check a Windows system | [Collect local GPU and driver information](#windows-inventory) |
| See tested hardware and measurements | [Hardware results](docs/HARDWARE-RESULTS.md) |
| Reproduce the 4096 × 4096 cuBLAS benchmark | [Benchmark setup and code](tools/CUBLAS-BENCHMARK.md) |
| Inspect pre-boot PCI information | [EFI probe guide](docs/EFI-PROBE.md) |
| Review the PCI transaction framework | [Experimental EFI transaction app](efi/GpuWriteExperiment/README.md) |

## RTX generation scope

The first implementation target is the exact Acer Nitro V 15 ANV15-41 / RTX
4050 Laptop GPU configuration (`10DE:28A1`, subsystem `1025:1758`, Ada / AD107M).
Other RTX 40 models and RTX 20 (Turing), RTX 30 (Ampere), and RTX 50 (Blackwell)
are future work.

| Priority | Target | Architecture | Machine |
|---|---|---|---|
| First target | RTX 4050 Laptop GPU | Ada Lovelace / AD107M | Acer Nitro V 15 ANV15-41 |
| Planned | Other RTX 40 models | Ada Lovelace | Separate profiles required |
| Planned | RTX 20, 30, and 50 series | Turing, Ampere, Blackwell | Future backends |

Compatibility discovery will use GPU architecture and hardware identity.

The repository includes Windows inventory tooling, a pre-boot PCI probe,
and a separate UEFI PCI transaction writer. The writer framework is present,
but the Acer RTX 4050 profile has no write actions, so this build does not yet
run the unlock procedure that produced the recorded benchmark result.

## Recorded benchmark results

| GPU | VRAM | PCI device / subsystem | System | FP16 before → after | BF16 before → after | Windows boot |
|---|---:|---|---|---:|---:|---|
| GeForce RTX 4050 Laptop GPU | 6 GB | `10DE:28A1` / `1025:1758` | Acer Nitro V 15 ANV15-41 | 41.57 → 78.87 TFLOPS | 41.51 → 78.67 TFLOPS | Successful |

The RTX 4050 result is a recorded benchmark on an Ada GPU. The measurements use CUDA 12.6 and cuBLAS 12.6 with a 4096 × 4096 GEMM, FP32 accumulation/output for FP16 and BF16 inputs, two seconds of warmup, 30 samples, and three trials. See [hardware test records](docs/HARDWARE-RESULTS.md).

## Windows inventory

Run from PowerShell in this directory:

```powershell
.\tools\collect-inventory.ps1
```

To save the report locally:

```powershell
.\tools\collect-inventory.ps1 -OutFile .\inventory.json
```

The collector records selected Windows, CPU, system/BIOS, display-adapter PCI ID, driver, and optional `nvidia-smi` fields. It omits hostnames, user names, serial numbers, UUIDs, and raw PnP instance suffixes.

## EFI and device profiles

- `GpuReadOnlyProbe` collects PCI identity, bridge ranges, BAR descriptors, Secure Boot, and same-handle GOP presence.
- `GpuWriteExperiment` is a write-capable PCI transaction framework. Its Acer RTX 4050 profile has the target identity recorded but zero actions and remains denied.
- The checked-in profile does not write hardware. The project does not flash VBIOS, replace NVIDIA drivers, install a Windows service, or change Windows boot configuration.

See [hardware results](docs/HARDWARE-RESULTS.md), [architecture](docs/ARCHITECTURE.md), [development roadmap](docs/ROADMAP.md), [profile evidence](docs/PROFILE-EVIDENCE.md), [transaction model](docs/TRANSACTION-MODEL.md), and [security boundaries](SECURITY.md).

## License

This project is licensed under the [MIT License](LICENSE).

