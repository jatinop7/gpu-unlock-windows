# Development stages

## Stage 0: Windows inventory

Collect a small, sanitized record of the OS, CPU, system/board/BIOS identity, display-adapter PCI IDs, driver versions, and optional nvidia-smi fields. It runs without elevation and does not inspect raw firmware or hardware registers.

## Stage 1: Acer RTX 4050 compatibility evidence

Start with the Acer Nitro V 15 ANV15-41 / RTX 4050 Laptop GPU (`10DE:28A1`, subsystem `1025:1758`). Record its VBIOS, system BIOS, integrated/discrete display routing, UEFI boot path, GPU cold-start state, power-gating behavior, and recovery route. Add other RTX 40 systems, then RTX 20/Turing, RTX 30/Ampere, and RTX 50/Blackwell.

## Stage 2: Pre-boot diagnostics

The EDK II application under efi/GpuProbe reads PCI identity, bridge bus ranges, BAR descriptors, Secure Boot, and same-handle GOP presence. It has no hardware write path. Complete protocol ownership and GPU registers are outside its current capture.

## Stage 3: Offline parsers and transaction model

The EFI capture parser, transaction trace auditor, and versioned schemas operate on local files and do not access hardware. The transaction model is an audit tool, not a runtime driver.

## Stage 4: Acer RTX 4050 backend and profile admission

The Acer RTX 4050 is the first implementation target. Its identity is in the EDK II profile, which remains denied with zero write actions. Implement the Ada backend and device discovery before publishing a runnable unlock path. Other RTX 40 systems and RTX 20/30/50 are future backend work.

## Stage 5: Expand GPU test coverage

A template and evidence audit record cold-boot runs, recovery, and repeated benchmark measurements. Add each GPU and exact system as a separate hardware result when tested.

The RTX 4050 result remains the recorded Ada benchmark. Add coverage for other RTX 40 systems, then expand to RTX 20, 30, and 50...
