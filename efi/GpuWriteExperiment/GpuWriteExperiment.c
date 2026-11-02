#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PciIo.h>

// This application is deliberately separate from GpuReadOnlyProbe. A profile
// must be reviewed and enabled in GpuWriteProfile.h before any write can occur.
#include "GpuWriteProfile.h"

#define PCI_CONFIG_EXTENDED_LIMIT 0x1000

typedef struct {
  EFI_PCI_IO_PROTOCOL *PciIo;
  EFI_HANDLE          Handle;
  UINTN               Segment;
  UINTN               Bus;
  UINTN               Device;
  UINTN               Function;
  UINT16              VendorId;
  UINT16              DeviceId;
  UINT16              SubsystemVendorId;
  UINT16              SubsystemDeviceId;
} GPU_TARGET;

typedef struct {
  UINT32 OldValue;
} GPU_SNAPSHOT;

STATIC
EFI_STATUS
ReadConfig32 (
  IN  EFI_PCI_IO_PROTOCOL *PciIo,
  IN  UINT16              Offset,
  OUT UINT32              *Value
  )
{
  if ((PciIo == NULL) || (Value == NULL) || ((Offset & 3) != 0) ||
      (Offset < 0x100) || (Offset > (PCI_CONFIG_EXTENDED_LIMIT - sizeof (UINT32))))
  {
    return EFI_INVALID_PARAMETER;
  }

  return PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, Offset, 1, Value);
}

STATIC
EFI_STATUS
WriteConfig32 (
  IN EFI_PCI_IO_PROTOCOL *PciIo,
  IN UINT16              Offset,
  IN UINT32              Value
  )
{
  if ((PciIo == NULL) || ((Offset & 3) != 0) || (Offset < 0x100) ||
      (Offset > (PCI_CONFIG_EXTENDED_LIMIT - sizeof (UINT32))))
  {
    return EFI_INVALID_PARAMETER;
  }

  return PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, Offset, 1, &Value);
}

STATIC
EFI_STATUS
FindExactTarget (
  OUT GPU_TARGET *Target
  )
{
  EFI_STATUS          Status;
  EFI_HANDLE          *Handles;
  UINTN               HandleCount;
  UINTN               Index;
  UINTN               Matches;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT32              Ids;
  GPU_TARGET          Candidate;

  if ((Target == NULL) || !mGpuWriteProfile.Admitted ||
      (mGpuWriteProfile.ActionCount == 0) || (mGpuWriteProfile.Actions == NULL))
  {
    return EFI_ACCESS_DENIED;
  }

  Handles     = NULL;
  HandleCount = 0;
  Matches     = 0;
  Status      = gBS->LocateHandleBuffer (
                       ByProtocol,
                       &gEfiPciIoProtocolGuid,
                       NULL,
                       &HandleCount,
                       &Handles
                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    PciIo = NULL;
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    if (EFI_ERROR (Status) || (PciIo == NULL)) {
      continue;
    }

    ZeroMem (&Candidate, sizeof (Candidate));
    Candidate.PciIo = PciIo;
    Candidate.Handle = Handles[Index];
    Status = PciIo->GetLocation (
                      PciIo,
                      &Candidate.Segment,
                      &Candidate.Bus,
                      &Candidate.Device,
                      &Candidate.Function
                      );
    if (EFI_ERROR (Status) ||
        (mGpuWriteProfile.MatchExactBdf &&
         ((Candidate.Segment != mGpuWriteProfile.Segment) ||
          (Candidate.Bus != mGpuWriteProfile.Bus) ||
          (Candidate.Device != mGpuWriteProfile.Device) ||
          (Candidate.Function != mGpuWriteProfile.Function))))
    {
      continue;
    }

    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, 1, &Ids);
    if (EFI_ERROR (Status)) {
      continue;
    }
    Candidate.VendorId = (UINT16)(Ids & 0xffff);
    Candidate.DeviceId = (UINT16)(Ids >> 16);

    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0x2c, 1, &Ids);
    if (EFI_ERROR (Status)) {
      continue;
    }
    Candidate.SubsystemVendorId = (UINT16)(Ids & 0xffff);
    Candidate.SubsystemDeviceId = (UINT16)(Ids >> 16);

    if ((Candidate.VendorId == mGpuWriteProfile.VendorId) &&
        (Candidate.DeviceId == mGpuWriteProfile.DeviceId) &&
        (Candidate.SubsystemVendorId == mGpuWriteProfile.SubsystemVendorId) &&
        (Candidate.SubsystemDeviceId == mGpuWriteProfile.SubsystemDeviceId))
    {
      *Target = Candidate;
      Matches++;
    }
  }

  if (Handles != NULL) {
    gBS->FreePool (Handles);
  }

  return (Matches == 1) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

STATIC
BOOLEAN
HasSameHandleGop (
  IN EFI_HANDLE Handle
  )
{
  EFI_STATUS                   Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

  Gop = NULL;
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop
                  );
  return !EFI_ERROR (Status) && (Gop != NULL);
}

STATIC
EFI_STATUS
ValidateProfile (
  VOID
  )
{
  UINTN Index;
  UINTN Other;
  CONST GPU_WRITE_ACTION *Action;

  if (!mGpuWriteProfile.Admitted || (mGpuWriteProfile.ActionCount == 0) ||
      (mGpuWriteProfile.Actions == NULL) || (mGpuWriteProfile.ProfileId == NULL) ||
      (mGpuWriteProfile.ProfileId[0] == L'\0') ||
      !mGpuWriteProfile.RecoveryTested ||
      !mGpuWriteProfile.IndependentRecoveryAvailable ||
      (mGpuWriteProfile.RecoveryMethod == NULL) ||
      (mGpuWriteProfile.RecoveryMethod[0] == L'\0'))
  {
    return EFI_ACCESS_DENIED;
  }

  for (Index = 0; Index < mGpuWriteProfile.ActionCount; Index++) {
    Action = &mGpuWriteProfile.Actions[Index];
    if (((Action->Offset & 3) != 0) || (Action->Offset < 0x100) ||
        (Action->Offset > (PCI_CONFIG_EXTENDED_LIMIT - sizeof (UINT32))) ||
        (Action->WriteMask == 0) || (Action->VerifyMask == 0) ||
        ((Action->ExpectedValue & ~Action->WriteMask) !=
         (Action->DesiredValue & ~Action->WriteMask)) ||
        !Action->RollbackSemanticsReviewed)
    {
      return EFI_INVALID_PARAMETER;
    }

    for (Other = Index + 1; Other < mGpuWriteProfile.ActionCount; Other++) {
      if (Action->Offset == mGpuWriteProfile.Actions[Other].Offset) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  return EFI_SUCCESS;
}

STATIC
VOID
StopOnUnsafeRollback (
  VOID
  )
{
  Print (L"\r\nROLLBACK FAILED. Do not continue boot. Power off fully and seek recovery help.\r\n");
  for (;;) {
    gBS->Stall (1000000);
  }
}

STATIC
BOOLEAN
ConfirmExperiment (
  VOID
  )
{
  EFI_INPUT_KEY Key;
  UINTN         EventIndex;
  UINTN         Index;
  UINT32        WriteValue;
  CONST GPU_WRITE_ACTION *Action;

  Print (L"Profile: %s\r\n", mGpuWriteProfile.ProfileId);
  Print (L"This will write volatile PCI extended configuration values.\r\n");
  for (Index = 0; Index < mGpuWriteProfile.ActionCount; Index++) {
    Action = &mGpuWriteProfile.Actions[Index];
    WriteValue = (Action->ExpectedValue & ~Action->WriteMask) |
                 (Action->DesiredValue & Action->WriteMask);
    Print (
      L"PCI CFG 0x%03x: 0x%08x -> 0x%08x (write mask 0x%08x)\r\n",
      (UINTN)Action->Offset,
      (UINTN)Action->ExpectedValue,
      (UINTN)WriteValue,
      (UINTN)Action->WriteMask
      );
  }
  Print (L"Press Y at the firmware prompt to continue; any other key cancels: ");
  if ((gST->ConIn == NULL) ||
      EFI_ERROR (gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex)) ||
      EFI_ERROR (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key)))
  {
    Print (L"input unavailable; cancelled.\r\n");
    return FALSE;
  }

  Print (L"%c\r\n", Key.UnicodeChar);
  return (Key.UnicodeChar == L'Y') || (Key.UnicodeChar == L'y');
}

STATIC
EFI_STATUS
RunTransaction (
  IN GPU_TARGET *Target
  )
{
  EFI_STATUS          Status;
  EFI_STATUS          RollbackStatus;
  UINTN               Index;
  UINTN               RollbackIndex;
  UINT32              Current;
  UINT32              Verify;
  UINT32              WriteValue;
  UINT32              RollbackValue;
  GPU_SNAPSHOT        *Snapshots;
  CONST GPU_WRITE_ACTION *Action;

  Snapshots = AllocateZeroPool (sizeof (*Snapshots) * mGpuWriteProfile.ActionCount);
  if (Snapshots == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Preflight every register before the first write. Values are compared as
  // full dwords; a profile must account for register-specific write semantics.
  for (Index = 0; Index < mGpuWriteProfile.ActionCount; Index++) {
    Action = &mGpuWriteProfile.Actions[Index];
    Status = ReadConfig32 (Target->PciIo, Action->Offset, &Current);
    if (EFI_ERROR (Status) || (Current != Action->ExpectedValue)) {
      Print (L"Preflight rejected at config offset 0x%03x. No writes made.\r\n", Action->Offset);
      FreePool (Snapshots);
      return EFI_ABORTED;
    }
    Snapshots[Index].OldValue = Current;
  }

  if (!ConfirmExperiment ()) {
    FreePool (Snapshots);
    return EFI_ABORTED;
  }

  for (Index = 0; Index < mGpuWriteProfile.ActionCount; Index++) {
    Action = &mGpuWriteProfile.Actions[Index];
    WriteValue = (Snapshots[Index].OldValue & ~Action->WriteMask) |
                 (Action->DesiredValue & Action->WriteMask);
    Status = WriteConfig32 (Target->PciIo, Action->Offset, WriteValue);
    if (!EFI_ERROR (Status)) {
      Status = ReadConfig32 (Target->PciIo, Action->Offset, &Verify);
    }
    if (!EFI_ERROR (Status) &&
        ((Verify & Action->VerifyMask) == (WriteValue & Action->VerifyMask)))
    {
      Print (
        L"Verified config offset 0x%03x: 0x%08x -> 0x%08x (write mask 0x%08x).\r\n",
        (UINTN)Action->Offset,
        (UINTN)Snapshots[Index].OldValue,
        (UINTN)Verify,
        (UINTN)Action->WriteMask
        );
      continue;
    }

    Print (L"Write/readback failed at config offset 0x%03x; rolling back.\r\n", Action->Offset);
    // Include the failing action: a failed status/readback does not prove the
    // device left its old value in place.
    RollbackIndex = Index + 1;
    while (RollbackIndex > 0) {
      RollbackIndex--;
      Action = &mGpuWriteProfile.Actions[RollbackIndex];
      RollbackStatus = ReadConfig32 (Target->PciIo, Action->Offset, &Current);
      if (!EFI_ERROR (RollbackStatus)) {
        RollbackValue = (Current & ~Action->WriteMask) |
                        (Snapshots[RollbackIndex].OldValue & Action->WriteMask);
        RollbackStatus = WriteConfig32 (Target->PciIo, Action->Offset, RollbackValue);
      }
      if (!EFI_ERROR (RollbackStatus)) {
        RollbackStatus = ReadConfig32 (Target->PciIo, Action->Offset, &Current);
      }
      if (EFI_ERROR (RollbackStatus) ||
          ((Current & Action->WriteMask) !=
           (Snapshots[RollbackIndex].OldValue & Action->WriteMask)))
      {
        FreePool (Snapshots);
        StopOnUnsafeRollback ();
      }
    }

    FreePool (Snapshots);
    return EFI_ABORTED;
  }

  FreePool (Snapshots);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  GPU_TARGET Target;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  Print (L"GPU_UNLOCK_EXPERIMENT|1\r\n");
  Status = ValidateProfile ();
  if (EFI_ERROR (Status)) {
    Print (L"WRITE_DISABLED|No reviewed, enabled hardware profile is present.\r\n");
    Print (L"This build performed no hardware writes.\r\n");
    return EFI_SUCCESS;
  }

  Status = FindExactTarget (&Target);
  if (EFI_ERROR (Status)) {
    Print (L"TARGET_REJECTED|Expected exactly one matching PCI function.\r\n");
    return EFI_SUCCESS;
  }

  if (HasSameHandleGop (Target.Handle)) {
    Print (L"TARGET_REJECTED|The GPU exposes GOP on its PCI handle.\r\n");
    return EFI_SUCCESS;
  }

  Print (
    L"TARGET|%04x:%04x|subsys=%04x:%04x|%04x:%02x:%02x.%x\r\n",
    (UINTN)Target.VendorId,
    (UINTN)Target.DeviceId,
    (UINTN)Target.SubsystemVendorId,
    (UINTN)Target.SubsystemDeviceId,
    Target.Segment,
    Target.Bus,
    Target.Device,
    Target.Function
    );

  Status = RunTransaction (&Target);
  Print (EFI_ERROR (Status) ? L"TRANSACTION|ABORTED_OR_ROLLED_BACK\r\n" : L"TRANSACTION|WRITES_VERIFIED\r\n");
  Print (L"No persistent firmware or VBIOS data was written.\r\n");
  return EFI_SUCCESS;
}
