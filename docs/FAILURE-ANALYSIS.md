# Failure analysis

## Reference run

The supplied EFI report records two writes, two passing readbacks, `EFI Status: SUCCESS`, and `Boot Handoff: SUCCESS`. The user-reported Windows boot after the experiment was successful. No failed write, failed readback, device reset, failed boot, display loss, or recovery event was reported for this run.

| Failure case | Recorded outcome |
|---|---|
| Profile/identity mismatch | No mismatch reported |
| Precondition value mismatch | No mismatch reported |
| PCI write failure | None reported |
| Readback failure | None reported; both checks passed |
| Rollback invoked | No; the transaction succeeded |
| EFI hang or device reset | None reported |
| Windows handoff / boot | Successful |

## Recovery route

The profile's independent recovery route is to power the laptop off fully, remove the EFI USB, and select Windows Boot Manager from the firmware boot menu. The recorded run reached Windows normally. Because no transaction failure occurred, the run did not exercise recovery from a failed write or a firmware lockup.

## Failure handling in the writer

The current writer checks for one exact target device and expected pre-write values before writing. It verifies each dword after writing. If a transaction step fails, it attempts reverse-order rollback of the masked bits and verifies those bits. If rollback cannot be verified, the application stops in firmware instead of handing off to Windows.

These code paths describe the current implementation; the supplied successful transaction log exercises the success path only. A failed write, rollback, or locked-device incident is not part of the reference run record.

## Related records

- Exact writes and readbacks: [EFI transactions](EFI-TRANSACTIONS.md).
- Standard interpretation of offsets `0x100` and `0x104`: [PCI register map](PCI-REGISTER-MAP.md).
- Power, clock, temperature, and throttle data: [power analysis](POWER-ANALYSIS.md).
