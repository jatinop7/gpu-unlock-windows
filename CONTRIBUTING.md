# Contributing

The project is early research. Contributions should make machine state easier to inspect and safety decisions easier to audit.

## Compatibility reports

Share only reviewed JSON from the inventory tool. Remove personal information first. Do not attach VBIOS/PROM dumps, firmware images, driver binaries, serial numbers, UUIDs, hostnames, usernames, or private filesystem paths.

For performance reports, include the exact GPU PCI and subsystem IDs, laptop or board model, BIOS version, NVIDIA driver, workload, precision mode, power limit, and repeated locked/unlocked measurements. A result applies only to the recorded machine and procedure.

## Development rules

- Keep inventory collection separate from hardware and firmware writes, and independent of unlock code.
- Keep machine-specific assumptions in explicit profiles; do not widen support from a GPU family name alone.
- Keep generated firmware and capture data out of Git.
- Do not describe a machine as supported until its complete transaction, recovery route, and benchmark evidence are documented.
- Do not submit successful reports as failure issues; use the project’s success-report channel once one exists.
