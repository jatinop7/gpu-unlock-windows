# Offline transaction model and profile gate

The transaction model audits a JSON event trace without controlling hardware:

    python -I tools/transaction_model.py trace.json

The model requires profile admission and preflight before a mutation request, then readback verification, bus-master disablement, zero outstanding DMA mappings, restoration of normal device ownership, and only then Windows handoff. A complete trace is still only a model result; it cannot prove that the events occurred on a physical machine.

The profile gate compares the Windows inventory with an EFI capture and the exact identities in profiles/registry.json:

    python -I tools/profile_gate.py inventory.json capture.json

The profile registry records the Acer RTX 4050 / Ada hardware and benchmark result. The offline registry remains measurement-only (`write_enabled=false`); its gate reports `can_unlock=false` and `write_permitted=false` and does not control hardware. Separately, the compiled `GpuWriteExperiment` profile is enabled and contains the two Acer PCI extended-configuration actions recorded in the hardware results. Other RTX 40 systems and RTX 20/30/50 support are future work.

## Conditions for adding actions to the in-tree transaction example

Adding or changing actions for another RTX 40/Ada system requires an exact
profile, cold-boot captures, an independent recovery procedure tested on the
target system, and hardware-specific cleanup evidence. Establish each
register's write and rollback semantics before adding it to a profile.
