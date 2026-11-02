#ifndef GPU_WRITE_PROFILE_H_
#define GPU_WRITE_PROFILE_H_

#include <Uefi.h>

typedef struct {
  UINT16   Offset;
  UINT32   ExpectedValue;
  UINT32   DesiredValue;
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

// Acer Nitro V 15 ANV15-41 / RTX 4050 identity is recorded. The current
// executor only writes PCI extended configuration space; no Ada unlock
// actions are defined here. Keep the profile denied until a real Ada backend,
// reviewed actions, and an independent recovery route exist.
STATIC CONST GPU_WRITE_PROFILE mGpuWriteProfile = {
  FALSE,
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
  FALSE,
  FALSE,
  L"",
  0,
  NULL
};

#endif
