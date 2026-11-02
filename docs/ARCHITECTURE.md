# Architecture direction

The project combines machine discovery and performance measurement with an experimental multi-generation RTX UEFI write framework. The Acer RTX 4050 profile contains two masked PCI extended-configuration actions; other GPU profiles remain future work.

## Components

1. **Windows inventory** — runs inside Windows and collects selected public hardware identifiers and software versions. It has no register or firmware access.
2. **Pre-boot diagnostics** — a separate EFI program observes PCI/resource/display facts before Windows drivers claim the GPU. It does not write hardware and remains separate from the transaction app.
3. **Offline preparation** — future parsers and profile generation operate on reviewed local captures and pinned public inputs. Generated firmware stays local.
4. **RTX write path** — the Acer Nitro V 15 ANV15-41 / RTX 4050 Laptop GPU profile is enabled with two recorded PCI extended-configuration actions. Other RTX 40 systems and RTX 20/30/50 are future targets.
5. **Windows handoff and measurement** — record Windows boot behavior and CUDA workloads for each exact system. The RTX 4050 Laptop GPU benchmark is retained in `docs/HARDWARE-RESULTS.md` as an RTX 40/Ada performance record.

## Profile and transaction rules

- A device family is not a machine profile. Profiles pin card, subsystem, firmware, BIOS, PCI path, display ownership, and relevant state together.
- The inventory report is never consumed as authorization to write hardware.
- Diagnostic and write-capable binaries must be distinguishable by build identity and behavior.
- Unknown state, unexpected power state, display ownership changes, or incomplete firmware halt/cleanup must stop the transaction.
- DMA-visible buffers remain owned until the firmware is known to have stopped; an uncertain stop cannot be treated as successful cleanup.
- Hand off only after selectors, PCI state, display recovery, and reset/cleanup state meet the exact profile’s evidence criteria.

## Current implementation

The repository contains the Windows inventory collector, EFI PCI probe, offline parsers and evidence tools, the cuBLAS benchmark, and a profile-gated PCI transaction writer. The RTX 4050 benchmark and its two-action transaction are recorded in the hardware results. The Acer RTX 4050 is the first enabled target; other RTX 40 models and RTX 20/30/50 are future work.
