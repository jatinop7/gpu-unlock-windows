# Reproducibility

## Reference machine inventory

The accompanying [system inventory](../artifacts/system-inventory.json) was collected from the Acer Nitro V 15 on 2026-11-02 with this repository's inventory script.

| Component | Recorded value |
|---|---|
| System | Acer Nitro V 15 / `Nitro ANV15-41` |
| CPU | AMD Ryzen 7 7735HS with Radeon Graphics |
| OS | Windows 10 Home Single Language, DisplayVersion `26H2`, build `26300.9711` |
| System BIOS | Insyde `V1.52`, release date `2026-08-04` |
| GPU | NVIDIA GeForce RTX 4050 Laptop GPU, Ada, compute capability `8.9` |
| GPU PCI identity | `10DE:28A1`, subsystem `1025:1758`, revision `A1` |
| VBIOS | `95.07.43.00.ae` |
| VRAM | `6141 MiB` |
| Current NVIDIA driver | `616.92` |

The original benchmark report specifies CUDA 12.6 and cuBLAS 12.6. It does not include the driver version or Windows build from the time of those measurements; the inventory above is the captured system snapshot.

## cuBLAS benchmark configuration

The included program is [`tools/cublas_4096_benchmark.cu`](../tools/cublas_4096_benchmark.cu). The reported run used:

| Setting | Value |
|---|---|
| Operation | cuBLAS GEMM, `4096 × 4096 × 4096` |
| CUDA / cuBLAS | CUDA 12.6 / cuBLAS 12.6 |
| Warmup | 2 seconds per precision |
| Samples | 30 event-timed samples per trial |
| Trials | 3 per precision |
| Reported statistic | Median of the three trial medians |
| FP16 mode | FP16 inputs, FP32 compute/accumulation, FP32 output |
| BF16 mode | BF16 inputs, FP32 compute/accumulation, FP32 output |
| FP32 mode | FP32 inputs, pedantic FP32 compute, FP32 output |
| TF32 mode | FP32 inputs, TF32 Tensor Core compute, FP32 output |

Build and run from a VS2022 Developer PowerShell with CUDA Toolkit 12.6:

```powershell
nvcc -std=c++17 .\tools\cublas_4096_benchmark.cu -o .\tools\cublas_4096_benchmark.exe -lcublas
.\tools\cublas_4096_benchmark.exe
```

The executable prints the GPU, CUDA runtime/driver, cuBLAS version, `NVIDIA_TF32_OVERRIDE`, trial medians, and final precision medians. The four available baseline/candidate reported scores are in [`artifacts/benchmark-raw.csv`](../artifacts/benchmark-raw.csv).

## EFI writer build

From an EDK II workspace containing this repository in a folder named `gpu-unlock-windows`, with the VS2022 toolchain initialized:

```text
build -p OvmfPkg/OvmfPkgX64.dsc -m gpu-unlock-windows/efi/GpuWriteExperiment/GpuWriteExperiment.inf -a X64 -t VS2022 -b DEBUG
```

The transaction report supplies build ID `EFI-RTX4050-2026-001` and version `0.1.0`. The report does not record a source commit or binary hash. [`artifacts/build-manifest.json`](../artifacts/build-manifest.json) identifies the repository profile commit and build recipe separately from those unrecorded binary details.

## Run sequence

1. Run the benchmark in the baseline state and retain its console output.
2. Boot the EFI writer from a separate FAT32 USB, confirm that the exact Acer profile and preconditions match, then press `Y` to apply the two recorded actions.
3. Allow the EFI application to hand off to Windows and confirm normal boot.
4. Run the same benchmark binary and configuration in the candidate state.
5. Capture power/clocks/temperature/throttle telemetry alongside each run using [power analysis](POWER-ANALYSIS.md).

The transaction values and the complete supplied EFI log are in [EFI transactions](EFI-TRANSACTIONS.md). The reported benchmark medians are FP16 `41.57 → 78.87 TFLOPS` and BF16 `41.51 → 78.67 TFLOPS`.
