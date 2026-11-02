#ifndef GPU_WRITE_PROFILE_H_
#define GPU_WRITE_PROFILE_H_

#include <Uefi.h>

typedef struct {
  UINT16   Offset;
  UINT32   ExpectedValue;
  UINT32   DesiredValue;
  UINT32   WriteMask;
  UINT32   VerifyMask;
  BOOLEAN  RollbackSemanticsReviewed;
} GPU_WRITE_ACTION;

typedef struct {
  BOOLEAN                  Admitted;
  CONST CHAR16             *ProfileId;
  UINTN                    Segment;
  UINTN                    Bus;
  UINTN                    Device;
  UINTN                    Function;
  BOOLEAN                  MatchExactBdf;
  UINT16                   VendorId;
  UINT16                   DeviceId;
  UINT16                   SubsystemVendorId;
  UINT16                   SubsystemDeviceId;
  BOOLEAN                  RecoveryTested;
  BOOLEAN                  IndependentRecoveryAvailable;
  CONST CHAR16             *RecoveryMethod;
  UINTN                    ActionCount;
  CONST GPU_WRITE_ACTION   *Actions;
} GPU_WRITE_PROFILE;

// Acer Nitro V 15 ANV15-41 / RTX 4050 action values are taken from the
// supplied successful EFI transaction report. Only the reported low nibble
// is changed; the executor preserves all other bits and rolls back that mask.
STATIC CONST GPU_WRITE_ACTION mAcerAnv15Rtx4050Actions[] = {
  { 0x100, 0x00000000, 0x00000005, 0x0000000f, 0xffffffff, TRUE },
  { 0x104, 0x00000010, 0x0000001a, 0x0000000f, 0xffffffff, TRUE }
};

STATIC CONST GPU_WRITE_PROFILE mGpuWriteProfile = {
  TRUE,
  L"acer-anv15-41-rtx4050-28a1",
  0,
  0,
  0,
  0,
  FALSE,
  0x10de,
  0x28a1,
  0x1025,
  0x1758,
  TRUE,
  TRUE,
  L"Power off fully, remove the EFI USB, and boot Windows Boot Manager.",
  sizeof (mAcerAnv15Rtx4050Actions) / sizeof (mAcerAnv15Rtx4050Actions[0]),
  mAcerAnv15Rtx4050Actions
};

#endif
