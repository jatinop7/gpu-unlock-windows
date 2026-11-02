#include <Uefi.h>

#include <Guid/GlobalVariable.h>
#include <IndustryStandard/Acpi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PciIo.h>

#define CONFIG_DWORD_COUNT 16
#define BAR_COUNT          6
#define PCI_CLASS_DISPLAY  0x03
#define PCI_CLASS_BRIDGE   0x06
#define PCI_SUBCLASS_PCI_BRIDGE 0x04

STATIC
CONST CHAR16 *
SecureBootState (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  UINT8       Value;

  Size   = sizeof (Value);
  Value  = 0;
  Status = gRT->GetVariable (
                  L"SecureBoot",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  &Value
                  );
  if (EFI_ERROR (Status) || (Size != sizeof (Value)) || (Value > 1)) {
    return L"unknown";
  }

  return (Value == 1) ? L"enabled" : L"disabled";
}

STATIC
CONST CHAR16 *
ResourceKind (
  IN UINT8 ResourceType
  )
{
  if (ResourceType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
    return L"MEM";
  }

  if (ResourceType == ACPI_ADDRESS_SPACE_TYPE_IO) {
    return L"IO";
  }

  if (ResourceType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
    return L"BUS";
  }

  return L"OTHER";
}

STATIC
VOID
PrintBarResources (
  IN EFI_PCI_IO_PROTOCOL *PciIo,
  IN UINTN               Segment,
  IN UINTN               Bus,
  IN UINTN               Device,
  IN UINTN               Function
  )
{
  EFI_STATUS                          Status;
  UINT8                               BarIndex;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Resource;

  for (BarIndex = 0; BarIndex < BAR_COUNT; BarIndex++) {
    Resource = NULL;
    Status   = PciIo->GetBarAttributes (
                        PciIo,
                        BarIndex,
                        NULL,
                        (VOID **)&Resource
                        );
    if (EFI_ERROR (Status) || (Resource == NULL)) {
      continue;
    }

    if (Resource->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
      Print (
        L"BAR|%04x|%02x|%02x|%x|%d|%s|%lx|%lx|%d\r\n",
        (UINTN)Segment,
        (UINTN)Bus,
        (UINTN)Device,
        (UINTN)Function,
        (UINTN)BarIndex,
        ResourceKind (Resource->ResType),
        Resource->AddrRangeMin,
        Resource->AddrLen,
        (UINTN)Resource->AddrSpaceGranularity
        );
    }

    FreePool (Resource);
  }
}

STATIC
VOID
PrintPciRecords (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   *Handles;
  UINTN                        HandleCount;
  UINTN                        Index;
  EFI_PCI_IO_PROTOCOL          *PciIo;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  UINTN                        Segment;
  UINTN                        Bus;
  UINTN                        Device;
  UINTN                        Function;
  UINT32                       Config[CONFIG_DWORD_COUNT];
  UINT8                        BaseClass;
  UINT8                        SubClass;
  UINT8                        ProgrammingInterface;
  UINT8                        Revision;
  UINT16                       VendorId;
  UINT16                       DeviceId;
  UINT16                       Command;
  UINT16                       StatusRegister;
  UINT8                        HeaderType;
  CONST CHAR16                 *GopOnSameHandle;

  Handles     = NULL;
  HandleCount = 0;
  Status      = gBS->LocateHandleBuffer (
                       ByProtocol,
                       &gEfiPciIoProtocolGuid,
                       NULL,
                       &HandleCount,
                       &Handles
                       );
  if (EFI_ERROR (Status)) {
    Print (L"ERROR|PCI_ENUMERATION|%r\r\n", Status);
    return;
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

    Status = PciIo->GetLocation (
                      PciIo,
                      &Segment,
                      &Bus,
                      &Device,
                      &Function
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (
                         PciIo,
                         EfiPciIoWidthUint32,
                         0,
                         CONFIG_DWORD_COUNT,
                         Config
                         );
    if (EFI_ERROR (Status)) {
      Print (
        L"ERROR|PCI_READ|%04x|%02x|%02x|%x|%r\r\n",
        (UINTN)Segment,
        (UINTN)Bus,
        (UINTN)Device,
        (UINTN)Function,
        Status
        );
      continue;
    }

    VendorId = (UINT16)(Config[0] & 0xffff);
    DeviceId = (UINT16)(Config[0] >> 16);
    if ((VendorId == 0xffff) || (VendorId == 0)) {
      continue;
    }

    Command       = (UINT16)(Config[1] & 0xffff);
    StatusRegister = (UINT16)(Config[1] >> 16);
    Revision      = (UINT8)(Config[2] & 0xff);
    ProgrammingInterface = (UINT8)((Config[2] >> 8) & 0xff);
    SubClass      = (UINT8)((Config[2] >> 16) & 0xff);
    BaseClass     = (UINT8)(Config[2] >> 24);
    HeaderType    = (UINT8)((Config[3] >> 16) & 0xff);

    if ((BaseClass != PCI_CLASS_DISPLAY) &&
        !((BaseClass == PCI_CLASS_BRIDGE) && (SubClass == PCI_SUBCLASS_PCI_BRIDGE)))
    {
      continue;
    }

    Gop = NULL;
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID **)&Gop
                    );
    GopOnSameHandle = EFI_ERROR (Status) ? L"no" : L"yes";

    Print (
      L"DEVICE|%04x|%02x|%02x|%x|%04x|%04x|%02x|%02x|%02x|%02x|%04x|%04x|%s|%02x\r\n",
      (UINTN)Segment,
      (UINTN)Bus,
      (UINTN)Device,
      (UINTN)Function,
      (UINTN)VendorId,
      (UINTN)DeviceId,
      (UINTN)BaseClass,
      (UINTN)SubClass,
      (UINTN)ProgrammingInterface,
      (UINTN)Revision,
      (UINTN)Command,
      (UINTN)StatusRegister,
      GopOnSameHandle,
      (UINTN)HeaderType
      );

    if ((BaseClass == PCI_CLASS_BRIDGE) && (SubClass == PCI_SUBCLASS_PCI_BRIDGE)) {
      Print (
        L"BRIDGE|%04x|%02x|%02x|%x|%02x|%02x|%02x\r\n",
        (UINTN)Segment,
        (UINTN)Bus,
        (UINTN)Device,
        (UINTN)Function,
        (UINTN)(Config[6] & 0xff),
        (UINTN)((Config[6] >> 8) & 0xff),
        (UINTN)((Config[6] >> 16) & 0xff)
        );
    } else if (BaseClass == PCI_CLASS_DISPLAY) {
      PrintBarResources (PciIo, Segment, Bus, Device, Function);
    }
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY Key;
  UINTN         EventIndex;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  Print (L"GPU_UNLOCK_DIAG|1\r\n");
  Print (L"SECURE_BOOT|%s\r\n", SecureBootState ());
  Print (L"NOTE|GOP is checked on the PCI handle only; this does not prove display ownership.\r\n");
  Print (L"NOTE|Observation probe: no PCI writes, MMIO access, PROM reads, file writes, or boot changes.\r\n");
  PrintPciRecords ();
  Print (L"END|1\r\n");
  Print (L"Copy the records above, then press any key to exit.\r\n");
  if (gST->ConIn != NULL) {
    Status = gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    if (!EFI_ERROR (Status)) {
      gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    }
  }
  return EFI_SUCCESS;
}
