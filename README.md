# Windows GPU Unlock

GPU performance and pre-boot research for Windows systems with NVIDIA RTX GPUs. The first machine target is the Acer Nitro V 15 ANV15-41 with its RTX 4050 Laptop GPU. Other GPU models are future work.

> **Project status:** The Acer Nitro V 15 RTX 4050 profile is enabled with two masked PCI extended-configuration writes from the recorded EFI transaction. It matches the PCI and subsystem IDs, checks expected register values, asks for confirmation, verifies readback, and attempts masked rollback on failure. Other GPU profiles are future work.

## Start here

| Goal | Guide |
|---|---|
| See the RTX generation scope | [RTX generation scope](#rtx-generation-scope) |
| Check a Windows system | [Collect local GPU and driver information](#windows-inventory) |
| Use a different RTX GPU | [Check compatibility and contribute a profile](#check-compatibility-for-another-rtx-gpu) |
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
and a separate UEFI PCI transaction writer. The enabled Acer RTX 4050 profile
contains the two recorded write actions at PCI configuration offsets `0x100`
and `0x104`; see the [transaction app guide](efi/GpuWriteExperiment/README.md).

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

## Check compatibility for another RTX GPU

The inventory and pre-boot probe steps below work for owners of RTX 20, 30,
40, and 50 series GPUs. Running the writer requires a matching enabled profile;
the only enabled profile currently targets the Acer Nitro V 15 ANV15-41 RTX
4050 (`10DE:28A1`, subsystem `1025:1758`). Other GPU models can use these steps
to collect information and contribute a profile.

1. Collect the Windows report:

   ```powershell
   .\tools\collect-inventory.ps1 -OutFile .\inventory.json
   ```

2. Build and run the EFI PCI probe from a separate FAT32 USB using the
   [probe guide](docs/EFI-PROBE.md). Keep its output and the inventory report
   local while reviewing them for personal information.
3. Compare the GPU PCI ID, subsystem ID, laptop or board, BIOS, VBIOS, and
   pre-boot device information with the [recorded profile evidence](docs/PROFILE-EVIDENCE.md)
   and [hardware results](docs/HARDWARE-RESULTS.md).
4. If your exact system has no enabled profile, submit the sanitized inventory
   and non-sensitive system details through the
   [compatibility issue form](.github/ISSUE_TEMPLATE/compatibility.yml). Do not
   edit the Acer IDs or action values to make another GPU match.
5. Launch `GpuWriteExperiment.efi` only when your exact GPU and subsystem IDs
   have an enabled profile and every preflight value matches. The app prints
   the write list and waits for `Y` before making changes; any other key exits.

An RTX generation name alone does not select a writer profile. RTX 20, 30,
other RTX 40, and RTX 50 profiles can be added as their exact configurations
and write sequences are established.

## EFI and device profiles

- `GpuReadOnlyProbe` collects PCI identity, bridge ranges, BAR descriptors, Secure Boot, and same-handle GOP presence.
- `GpuWriteExperiment` has an enabled Acer RTX 4050 profile with two masked writes, exact-value preflight, operator confirmation, readback verification, and rollback.
- The project does not flash VBIOS, replace NVIDIA drivers, install a Windows service, or change Windows boot configuration.

See [hardware results](docs/HARDWARE-RESULTS.md), [architecture](docs/ARCHITECTURE.md), [development roadmap](docs/ROADMAP.md), [profile evidence](docs/PROFILE-EVIDENCE.md), [transaction model](docs/TRANSACTION-MODEL.md), and [security boundaries](SECURITY.md).

## License

This project is licensed under the [MIT License](LICENSE).

