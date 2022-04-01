#include "pch.h"
#include "OBCallback.h"
#include "FSFilter.h"
#include "RegFilter.h"
#include "IPC.h"

//////////////////////////////////////////////////////////////////////////
// Global data

PDRIVER_OBJECT g_pDriverObject = NULL;
BOOLEAN g_bExit = FALSE;

//
// Function declarations
//
DRIVER_INITIALIZE  DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH TdDeviceCreate;
_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH TdDeviceClose;

DRIVER_UNLOAD   TdDeviceUnload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateSharedMemory)
#pragma alloc_text(PAGE, DataInsertThread)
#pragma alloc_text(PAGE, CreateData)
#pragma alloc_text(PAGE, POPDataThread)
#pragma alloc_text(PAGE, CallbackMonitor)
#endif

//
// DriverEntry
//

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT CallbackVersion;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ProcMon DriverEntry\n");

    g_pDriverObject = DriverObject;

    CallbackVersion = ObGetFilterVersion();

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
        "ObCallback version 0x%hx\n", CallbackVersion);

    DriverObject->DriverUnload = TdDeviceUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = TdDeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = TdDeviceClose;

    CreateSharedMemory();

    //Status = FsFilterInit(DriverObject);
    //if (!NT_SUCCESS(Status))
    //{
    //    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
    //        "FsFilterInit fail : 0x%x\n", Status);
    //}

    //Status = TdInitOBCallback();
    //if (!NT_SUCCESS(Status))
    //{
    //    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
    //        "TdInitOBCallback fail : 0x%x\n", Status);
    //}

    Status = RegFilterInit(DriverObject);
    if (!NT_SUCCESS(Status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "RegFilterInit fail : 0x%x\n", Status);
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ProcMon DriverEntry: End 0x%x\n", Status);

    //IOInit();

    return Status;
}

//
// Function:
//
//     TdDeviceUnload
//
// Description:
//
//     This function handles driver unloading. All this driver needs to do 
//     is to delete the device object and the symbolic link between our 
//     device name and the Win32 visible name.
//

VOID
TdDeviceUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    NTSTATUS Status = STATUS_SUCCESS;

    g_bExit = TRUE;

    ClearSharedMemory();

    //
    // remove any OB callbacks
    //
    //Status = TdDeleteOBCallback();
    //TD_ASSERT(Status == STATUS_SUCCESS);

    Status = RegFilterUnload(DriverObject);
    if (!NT_SUCCESS(Status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "RegFilterUnload fail : 0x%x\n", Status);
    }

    //Status = FsFilterUnload(DriverObject);
    //if (!NT_SUCCESS(Status))
    //{
    //    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
    //        "FsFilterUnload fail : 0x%x\n", Status);
    //}

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ProcMon: TdDeviceUnload End : 0x%x\n", Status);
}

//
// Function:
//
//     TdDeviceCreate
//
// Description:
//
//     This function handles the 'create' irp.
//


NTSTATUS
TdDeviceCreate(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

//
// Function:
//
//     TdDeviceClose
//
// Description:
//
//     This function handles the 'close' irp.
//

NTSTATUS
TdDeviceClose(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

//
// Function:
//
//     TdDeviceCleanup
//
// Description:
//
//     This function handles the 'cleanup' irp.
//

NTSTATUS
TdDeviceCleanup(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
