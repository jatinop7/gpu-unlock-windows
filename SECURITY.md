# Safety and security boundaries

This repository contains Windows inventory and diagnostic tools plus a separate experimental UEFI PCI transaction writer. The compiled Acer RTX 4050 profile is admitted and contains two masked PCI extended-configuration writes. The app requires an exact PCI/subsystem match, expected-value preflight, operator confirmation, and verifies readback; it attempts rollback if a write fails. Treat this build as hardware-facing code. The project does not install a boot application, alter EFI entries, or communicate over the network.

Future hardware profiles must remain disabled until their exact device, firmware, display path, cold state, DMA behavior, cleanup path, and recovery procedure have been reviewed. Unknown or inconsistent state must stop the transaction. Source compilation and simulated tests are not hardware validation.

Do not use experimental EFI code on a machine whose exact profile and recovery route have not been reviewed. Do not flash a VBIOS or replace the NVIDIA driver as part of this project.

Report suspected vulnerabilities or unsafe behavior privately to the project maintainer before publishing a working exploit or device-specific write sequence.
