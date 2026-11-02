# Machine profile evidence

Profiles are machine records, not product-family promises. A future profile should contain only the minimum data needed to make a compatibility decision:

- GPU vendor/device ID, subsystem ID, revision, architecture/chip identity, and reported VRAM size.
- Laptop/board model, BIOS vendor/version, and a hash of public firmware-facing inputs where appropriate. Do not publish raw VBIOS/PROM images.
- UEFI boot mode, PCI path, BAR layout, integrated-GPU and physical display ownership, and the exact pre-boot state observed.
- NVIDIA driver and GPU power-management behavior across cold boot, idle, sleep, and resume.
- Parser output, diagnostic logs, transaction/cleanup evidence, recovery route, and benchmark command/results.

Separate observations from inference. Windows inventory cannot establish the UEFI GOP owner, GPU cold state, protected-memory layout, DMA safety, or whether selector values survive the driver handoff. Those require dedicated evidence.

The packet must also document a recovery method independent of the experimental EFI app and record that it was tested on the exact laptop, followed by a normal boot. This reduces uncertainty but cannot prove recovery from a hard hardware hang.

Two independent cold-start captures should agree before a profile is used to generate hardware-facing code. Captures stay local by default. A profile remains experimental until its transaction and recovery path have been exercised on that exact machine and its results have been reviewed.
