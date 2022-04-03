#include "pch.h"
#include "OBCallback.h"
#include "FSFilter.h"
#include "RegFilter.h"
#include "IPC.h"

//////////////////////////////////////////////////////////////////////////
// Global data

PDRIVER_OBJECT g_pDriverObject = NULL;
PDEVICE_OBJECT g_pDeviceObject = NULL;
BOOLEAN g_bExit = FALSE;
BOOLEAN g_bObCallBack = FALSE;
BOOLEAN g_bFsCallBack = FALSE;
BOOLEAN g_bRegCallBack = FALSE;

//
// Function declarations
//
DRIVER_INITIALIZE  DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH TdDeviceCreate;
_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH TdDeviceClose;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH DeviceControl;

DRIVER_UNLOAD   TdDeviceUnload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IPC_Init)
#pragma alloc_text(PAGE, CreateData)
#pragma alloc_text(PAGE, POPDataThread)
#pragma alloc_text(PAGE, DeviceControl)
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

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ProcMon DriverEntry\n");

    g_pDriverObject = DriverObject;

    UNICODE_STRING NtDeviceName;
    UNICODE_STRING DosDevicesLinkName;

    //
    // Create our device object.
    //
    RtlInitUnicodeString(&NtDeviceName, NT_DEVICE_NAME);
    Status = IoCreateDevice(
        DriverObject,                 // pointer to driver object
        0,                            // device extension size
        &NtDeviceName,                // device name
        FILE_DEVICE_UNKNOWN,          // device type
        0,                            // device characteristics
        FALSE,                         // not exclusive
        &g_pDeviceObject);                // returned device object pointer

    if (!NT_SUCCESS(Status)) {
        DbgPrint("DriverEntry, IoCreateDevice Fail : 0x%x\n", Status);
        return Status;
    }

    //
    // Create a link in the Win32 namespace.
    //
    RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICE_NAME);
    Status = IoCreateSymbolicLink(&DosDevicesLinkName, &NtDeviceName);
    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice(g_pDeviceObject);
        return Status;
    }

    DriverObject->DriverUnload = TdDeviceUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = TdDeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = TdDeviceClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ProcMon DriverEntry Success\n");

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
    UNICODE_STRING  DosDevicesLinkName;

    RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&DosDevicesLinkName);

    IoDeleteDevice(g_pDeviceObject);

    g_bExit = TRUE;

    //
    // remove any OB callbacks
    //
    if (g_bObCallBack)
    {
        Status = TdDeleteOBCallback();
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("TdDeleteOBCallback Fail : 0x%x", Status);
        }
    }

    if (g_bRegCallBack)
    {
        Status = RegFilterUnload(DriverObject);
        if (!NT_SUCCESS(Status))
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                "RegFilterUnload fail : 0x%x\n", Status);
        }
    }

    if (g_bFsCallBack)
    {
        Status = FsFilterUnload(DriverObject);
        if (!NT_SUCCESS(Status))
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                "FsFilterUnload fail : 0x%x\n", Status);
        }
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ProcMon: TdDeviceUnload End : 0x%x\n", Status);
}

NTSTATUS
DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  irpSp;// Pointer to current stack location
    NTSTATUS            ntStatus = STATUS_SUCCESS;// Assume success
    ULONG               inBufLength; // Input buffer length
    ULONG               outBufLength; // Output buffer length
    PVOID               outBuf; // pointer to Input and output buffer
    PioCallbackControl pioControl = NULL;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    DbgPrint("ioctl\n");

    if (!inBufLength || !outBufLength)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    if (inBufLength != sizeof(ioCallbackControl))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    pioControl = Irp->AssociatedIrp.SystemBuffer;
    outBuf = Irp->AssociatedIrp.SystemBuffer;

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("main, DeviceControl, IPC_Init Fail : 0x%x\n", Status);
        goto End;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_CALLBACK_START:
        Status = IPC_Init(pioControl->Type, pioControl->CallbackPrefix);
        switch (pioControl->Type)
        {
        case 0:
            Status = FsFilterInit(g_pDriverObject);
            if (!NT_SUCCESS(Status))
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                    "FsFilterInit fail : 0x%x\n", Status);
            }
            else
            {
                g_bFsCallBack = TRUE;
                DbgPrint("FileSystem Filter Success\n");
            }
            break;
        case 1:
            Status = TdInitOBCallback();
            if (!NT_SUCCESS(Status))
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                    "TdInitOBCallback fail : 0x%x\n", Status);
            }
            else
            {
                g_bObCallBack = TRUE;
                DbgPrint("OB Callback Success\n");
            }
            break;
        case 2:
            Status = RegFilterInit(g_pDriverObject);
            if (!NT_SUCCESS(Status))
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                    "RegFilterInit fail : 0x%x\n", Status);
            }
            else
            {
                g_bRegCallBack = TRUE;
                DbgPrint("Registry Filter Success\n");
            }
            break;
        default:
            break;
        }
        break;

    case IOCTL_CALLBACK_STOP:
        switch (pioControl->Type)
        {
        case 0:
            if (g_bObCallBack)
            {
                Status = TdDeleteOBCallback();
                if (!NT_SUCCESS(Status))
                {
                    DbgPrint("TdDeleteOBCallback Fail : 0x%x", Status);
                }
                else
                {
                    g_bObCallBack = FALSE;
                    DbgPrint("OB Callback Remove\n");
                }
            }
            break;
        case 1:
            if (g_bRegCallBack)
            {
                Status = RegFilterUnload(g_pDriverObject);
                if (!NT_SUCCESS(Status))
                {
                    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                        "RegFilterUnload fail : 0x%x\n", Status);
                }
                else
                {
                    g_bRegCallBack = FALSE;
                    DbgPrint("Registry Callback Remove\n");
                }
            }
            break;
        case 2:
            if (g_bFsCallBack)
            {
                Status = FsFilterUnload(g_pDriverObject);
                if (!NT_SUCCESS(Status))
                {
                    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                        "FsFilterUnload fail : 0x%x\n", Status);
                }
                else
                {
                    g_bFsCallBack = FALSE;
                    DbgPrint("File Callback Remove\n");
                }
            }
            break;
        default:
            break;
        }
        break;
    case IOCTL_TEST:
        DbgPrint("Type : %ld\n", pioControl->Type);
        DbgPrint("Prefix : %ws\n", pioControl->CallbackPrefix);
        Status = STATUS_SUCCESS;
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
        break;
    }

    //
    // Write to the buffer over-writes the input buffer content
    //
    RtlCopyBytes(outBuf, &Status, sizeof(NTSTATUS));

    //
    // Assign the length of the data copied to IoStatus.Information
    // of the Irp and complete the Irp.
    //
    Irp->IoStatus.Information = sizeof(NTSTATUS);

End:

    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
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
