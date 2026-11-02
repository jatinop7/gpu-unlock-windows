# Offline transaction model and profile gate

The transaction model audits a JSON event trace without controlling hardware:

    python -I tools/transaction_model.py trace.json

The model requires profile admission and preflight before a mutation request, then readback verification, bus-master disablement, zero outstanding DMA mappings, restoration of normal device ownership, and only then Windows handoff. A complete trace is still only a model result; it cannot prove that the events occurred on a physical machine.

The profile gate compares the Windows inventory with an EFI capture and the exact identities in profiles/registry.json:

    python -I tools/profile_gate.py inventory.json capture.json

The profile registry records the Acer RTX 4050 / Ada hardware and benchmark result. It is measurement-only (`write_enabled=false`). The compiled profile now identifies the Acer 4050, but has zero actions and remains denied. The gate reports `can_unlock=false` and `write_permitted=false`; it audits local evidence and does not control hardware. `GpuWriteExperiment` has a generic PCI extended-configuration executor, not the Acer RTX 4050 unlock backend. Other RTX 40 systems and RTX 20/30/50 support are future work.

## Conditions for adding actions to the in-tree transaction example

Adding RTX 40/Ada actions to the in-tree `GpuWriteExperiment` requires a reviewed
exact profile, cold-boot captures, an independent recovery procedure tested on
the target system, an explicit default-off gate, and hardware-specific cleanup
evidence. Each register's write and rollback semantics must be established
before adding it to a profile.
