# Changelog

## Unreleased

- Add and enable the Acer Nitro V 15 RTX 4050 two-action PCI configuration profile.
- Apply the recorded low-nibble write masks and use masked rollback with full-dword readback verification.
- Document the EFI transaction alongside the RTX 4050 benchmark result.

## 0.1.0 — initial project implementation

- Create an independent Windows GPU performance research project.
- Add a local inventory collector and versioned JSON report schema.
- Add the 4096 × 4096 CUDA/cuBLAS benchmark and record FP16/BF16 before-and-after results.
- Record the RTX 4050 Laptop GPU 6 GB Windows boot and benchmark result.
- Add EFI diagnostics, a profile-gated PCI transaction framework, offline evidence tooling, and staged hardware research docs.
- Set RTX 40/Ada as the first implementation target; list RTX 20/Turing, RTX 30/Ampere, and RTX 50/Blackwell as future targets.
