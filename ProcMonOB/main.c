#include <ntifs.h>
#include ".\..\DriverCommon.h"
#include ".\..\ProcMonDriver\IPC.h"
#include ".\..\ProcMonDriver\OBCallback.h"

#define NT_DEVICE_NAME L"\\Device\\ProcMonOB"
#define DOS_DEVICE_NAME L"\\DosDevices\\ProcMonDeviceOB"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IPC_Init)
#pragma alloc_text(PAGE, CreateData)
#pragma alloc_text(PAGE, POPDataThread)
#pragma alloc_text(PAGE, CBTdPreOperationCallback)
#endif

extern BOOLEAN g_bObCallBack;

VOID
DeviceUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    NTSTATUS Status;
    LARGE_INTEGER interval;

    interval.QuadPart = (1 * -10 * 1000 * 3000);

    g_bObCallBack = FALSE;
    Status = TdDeleteOBCallback();
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("TdDeleteOBCallback Fail : 0x%x", Status);
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

    DbgPrint("ProcMonOB DriverEntry\n");

    SetOBName(NT_DEVICE_NAME, DOS_DEVICE_NAME);

    Status = IPC_Init(0, OBPrefix);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("IPC_Init : 0x%x\n", Status);
        goto End;
    }

    Status = TdInitOBCallback();
    if (!NT_SUCCESS(Status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "TdInitOBCallback fail : 0x%x\n", Status);
        goto End;
    }

    DriverObject->DriverUnload = DeviceUnload;

End:
    if (!NT_SUCCESS(Status))
    {
        g_bObCallBack = FALSE;
    }

    DbgPrint("ProcMonOB DriverEntry : 0x%x\n", Status);

    return Status;
}
