#include <ntifs.h>
#include ".\..\DriverCommon.h"
#include ".\..\ProcMonDriver\IPC.h"
#include ".\..\ProcMonDriver\RegFilter.h"

#define NT_DEVICE_NAME L"\\Device\\ProcMonREG"
#define DOS_DEVICE_NAME L"\\DosDevices\\ProcMonDeviceREG"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IPC_Init)
#pragma alloc_text(PAGE, CreateData)
#pragma alloc_text(PAGE, POPDataThread)
#pragma alloc_text(PAGE, CallbackMonitor)
#endif

extern BOOLEAN g_bRegCallBack;

VOID
DeviceUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS Status;
    LARGE_INTEGER interval;

    interval.QuadPart = (1 * -10 * 1000 * 3000);

    g_bRegCallBack = FALSE;
    Status = RegFilterUnload(DriverObject);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("RegFilterUnload Fail : 0x%x", Status);
    }

    KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS Status = STATUS_SUCCESS;

    DbgPrint("ProcMonREG DriverEntry\n");

    SetREGName(NT_DEVICE_NAME, DOS_DEVICE_NAME);

    Status = IPC_Init(2, REGPrefix);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("main, DeviceControl, IPC_Init Fail : 0x%x\n", Status);
        goto End;
    }

    Status = RegFilterInit(DriverObject);
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
        g_bRegCallBack = FALSE;
    }

    return Status;
}
