#include <ntifs.h>
#include ".\..\DriverCommon.h"
#include ".\..\ProcMonDriver\IPC.h"
#include ".\..\ProcMonDriver\FSFilter.h"

#define NT_DEVICE_NAME L"\\Device\\ProcMonFSDriver"
#define DOS_DEVICE_NAME L"\\DosDevices\\ProcMonDeviceFS"
#define FSPrefix L"fsprefix"

extern BOOLEAN g_bFsCallBack;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS Status = STATUS_SUCCESS;

    DbgPrint("ProcMonFS DriverEntry\n");

    Status = IPC_Init(1, FSPrefix);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("main, DeviceControl, IPC_Init Fail : 0x%x\n", Status);
        goto End;
    }

    Status = FsFilterInit(DriverObject);
    if (!NT_SUCCESS(Status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "FsFilterInit fail : 0x%x\n", Status);
        goto End;
    }

End:
    if (!NT_SUCCESS(Status))
    {
        g_bFsCallBack = FALSE;
    }

    DbgPrint("end\n");

    return Status;
}
