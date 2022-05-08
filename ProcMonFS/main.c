#include <ntifs.h>
#include ".\..\DriverCommon.h"
#include ".\..\ProcMonDriver\IPC.h"
#include ".\..\ProcMonDriver\FSFilter.h"

#define NT_DEVICE_NAME L"\\Device\\ProcMonOB"
#define DOS_DEVICE_NAME L"\\DosDevices\\ProcMonDeviceOB"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IPC_Init)
#pragma alloc_text(PAGE, CreateData)
#pragma alloc_text(PAGE, POPDataThread)
#pragma alloc_text(PAGE, FsFilterDispatchCreate)
#endif

extern BOOLEAN g_bFsCallBack;

VOID
DeviceUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS Status;
    LARGE_INTEGER interval;

    interval.QuadPart = (1 * -10 * 1000 * 3000);

    g_bFsCallBack = FALSE;
    Status = FsFilterUnload(DriverObject);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("FsFilterUnload Fail : 0x%x", Status);
    }

    KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

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

    SetFSName(NT_DEVICE_NAME, DOS_DEVICE_NAME);

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

    DriverObject->DriverUnload = DeviceUnload;

End:
    if (!NT_SUCCESS(Status))
    {
        g_bFsCallBack = FALSE;
    }

    DbgPrint("ProcMonFS DriverEntry : 0x%x\n", Status);

    return Status;
}
