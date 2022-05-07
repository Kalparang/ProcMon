#include <ntifs.h>
#include ".\..\DriverCommon.h"

#define NT_DEVICE_NAME L"\\Device\\ProcMonOB"
#define DOS_DEVICE_NAME L"\\DosDevices\\ProcMonDeviceOB"

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DbgPrint("ProcMonOB DriverEntry\n");

    SetDriverName(NT_DEVICE_NAME, DOS_DEVICE_NAME);
    SetOBName(L"\\Device\\ProcMonOB", L"\\DosDevices\\ProcMonoby");

    Status = ProcMonDriverEntry(DriverObject, RegistryPath);

    return Status;
}
