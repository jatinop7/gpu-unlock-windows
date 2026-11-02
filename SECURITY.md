# Safety and security boundaries

This repository contains Windows inventory and diagnostic tools plus a separate experimental UEFI PCI transaction writer. The compiled profile identifies the Acer RTX 4050 but contains no write actions and is denied, so the checked-in build exits without writes. The writer can apply PCI extended-configuration values when built with an admitted profile; treat any such build as hardware-facing code. The project does not install a boot application, alter EFI entries, or communicate over the network.

Future hardware work may execute before Windows starts and may write GPU registers. That work must remain disabled until the exact device, firmware, display path, cold state, DMA behavior, cleanup path, and recovery procedure have been reviewed. Unknown or inconsistent state must stop the transaction. Source compilation and simulated tests are not hardware validation.

Do not use experimental EFI code on a machine whose exact profile and recovery route have not been reviewed. Do not flash a VBIOS or replace the NVIDIA driver as part of this project.

Report suspected vulnerabilities or unsafe behavior privately to the project maintainer before publishing a working exploit or device-specific write sequence.
