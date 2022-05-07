#include <ntifs.h>
#include ".\..\DriverCommon.h"

#define NT_DEVICE_NAME L"\\Device\\ProcMonREG"
#define DOS_DEVICE_NAME L"\\DosDevices\\ProcMonDeviceREG"

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DbgPrint("ProcMonREG DriverEntry\n");

    SetDriverName(NT_DEVICE_NAME, DOS_DEVICE_NAME);
    SetREGName(L"\\Device\\ProcMonRF", L"\\DosDevices\\ProcMonreg");

    Status = ProcMonDriverEntry(DriverObject, RegistryPath);

    return Status;
}
