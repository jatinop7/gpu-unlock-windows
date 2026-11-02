# Windows GPU Unlock

An open-source toolkit for investigating and tuning NVIDIA RTX GPU performance on Windows. It combines Windows hardware inventory, pre-boot PCI diagnostics, a profile-based UEFI transaction app, and a CUDA/cuBLAS benchmark.

The RTX 4050 is the project's **reference configuration**, not the limit of the toolkit. The inventory, diagnostic, and benchmark tools are useful across RTX generations. Hardware writes are selected by exact device profiles: the only enabled write profile today is for the Acer Nitro V 15 ANV15-41 with an RTX 4050 Laptop GPU (`10DE:28A1`, subsystem `1025:1758`).

## What you can use today

| Your GPU | Inventory and PCI diagnostics | cuBLAS benchmark | Enabled EFI write profile |
|---|---:|---:|---:|
| RTX 20, 30, 40, or 50 series | Yes | Yes, with CUDA/cuBLAS installed | Only the exact Acer Nitro V 15 RTX 4050 configuration below |
| Other RTX models and systems | Yes | Yes, with CUDA/cuBLAS installed | No profile yet; contribute your hardware details to help add one |

The writer checks the GPU and subsystem IDs and the expected pre-write values before it offers the write action. A GPU being from the same RTX generation does not make it compatible with another system's profile.

## Get started

### 1. Download the project

```powershell
git clone https://github.com/jatinop7/gpu-unlock-windows.git
cd gpu-unlock-windows
```

### 2. Collect your GPU and system information

From PowerShell in the repository folder:

```powershell
.\tools\collect-inventory.ps1 -OutFile .\inventory.json
```

The report includes selected GPU, PCI, driver, Windows, and system/BIOS fields. Review it before sharing; the collector omits hostnames, usernames, serial numbers, UUIDs, and raw PnP instance suffixes.

### 3. Measure GPU performance

Use the [4096 × 4096 CUDA/cuBLAS benchmark guide](tools/CUBLAS-BENCHMARK.md) to build and run the benchmark on a CUDA-capable NVIDIA GPU. It records FP16, BF16, TF32, and FP32 results for comparison.

### 4. Inspect the GPU before Windows starts (optional)

The read-only EFI probe reports PCI identity, bridge ranges, BAR descriptors, Secure Boot state, and GOP presence. Build and run it from a configured EDK II workspace using the [EFI probe guide](docs/EFI-PROBE.md). Copy the resulting application to a separate FAT32 USB and launch it from your firmware's one-time boot menu.

### 5. Use a matching write profile (Acer reference system only)

The experimental writer currently supports only the exact Acer Nitro V 15 ANV15-41 RTX 4050 configuration listed above. It is built from source in a configured EDK II workspace with VS2022:

```text
build -p OvmfPkg/OvmfPkgX64.dsc -m gpu-unlock-windows/efi/GpuWriteExperiment/GpuWriteExperiment.inf -a X64 -t VS2022 -b DEBUG
```

Copy the generated `GpuWriteExperiment.efi` to a separate FAT32 USB and launch it from the firmware boot menu. The app displays its profile and write values, checks the exact GPU identity and expected register values, then waits for `Y`; any other key cancels. See the [transaction app guide](efi/GpuWriteExperiment/README.md) for the actions and recovery route.

Do not run this writer on another laptop or GPU by changing the profile IDs. For unsupported systems, use the inventory and read-only probe, then submit sanitized details with the [compatibility report form](.github/ISSUE_TEMPLATE/compatibility.yml).

## Reference system and results

The current reference profile is the Acer Nitro V 15 ANV15-41 with a GeForce RTX 4050 Laptop GPU, 6 GB VRAM, PCI ID `10DE:28A1`, and subsystem ID `1025:1758`. Its recorded 4096 × 4096 cuBLAS GEMM results are:

| Precision | Before | After |
|---|---:|---:|
| FP16 | 41.57 TFLOPS | 78.87 TFLOPS |
| BF16 | 41.51 TFLOPS | 78.67 TFLOPS |

The run used CUDA 12.6, cuBLAS 12.6, FP32 accumulation/output for FP16 and BF16 inputs, two seconds of warmup, 30 samples per precision, and three trials. See [hardware results](docs/HARDWARE-RESULTS.md) for the profile and benchmark record.

## Add support for another GPU

The project is organized around per-system profiles so that contributors can add support across RTX 20, 30, 40, and 50 series. Start by collecting the Windows inventory and read-only EFI probe output for the exact laptop or desktop. Share sanitized reports through the [compatibility report form](.github/ISSUE_TEMPLATE/compatibility.yml); include the GPU and subsystem IDs, system model, BIOS/VBIOS versions, driver, benchmark setup, and boot outcome. New write actions belong in a profile for the exact hardware configuration.

## Project guides

- [Hardware results and reference profile](docs/HARDWARE-RESULTS.md)
- [Unlock mechanism hypothesis](docs/UNLOCK-MECHANISM.md)
- [PCI register map](docs/PCI-REGISTER-MAP.md)
- [EFI transaction log](docs/EFI-TRANSACTIONS.md)
- [Power analysis](docs/POWER-ANALYSIS.md)
- [Reproducibility](docs/REPRODUCIBILITY.md)
- [Failure analysis and recovery](docs/FAILURE-ANALYSIS.md)
- [EFI PCI probe](docs/EFI-PROBE.md)
- [EFI transaction model](docs/TRANSACTION-MODEL.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Security boundaries](SECURITY.md)

The reference evidence files are [`artifacts/build-manifest.json`](artifacts/build-manifest.json), [`artifacts/benchmark-raw.csv`](artifacts/benchmark-raw.csv), and [`artifacts/system-inventory.json`](artifacts/system-inventory.json).

## License

This project is licensed under the [MIT License](LICENSE).
