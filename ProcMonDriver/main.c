#include "pch.h"
#include "OBCallback.h"
#include "FSFilter.h"
#include "RegFilter.h"
#include "IPC.h"
#include ".\..\DriverCommon.h"

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
//DRIVER_INITIALIZE  ProcMonDriverEntry;
//DRIVER_UNLOAD   ProcMonDeviceUnload;

VOID
ProcMonDeviceUnload(
    _In_ PDRIVER_OBJECT DriverObject
);

NTSTATUS
ProcMonDispatchPassthrough(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
);

NTSTATUS
ProcMonDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

//_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH ProcMonDeviceCreate;
//_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH ProcMonDeviceClose;
//_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH ProcMonDeviceControl;


//#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, IPC_Init)
//#pragma alloc_text(PAGE, CreateData)
//#pragma alloc_text(PAGE, POPDataThread)
//#pragma alloc_text(PAGE, DeviceControl)
//#pragma alloc_text(PAGE, CallbackMonitor)
//#pragma alloc_text(PAGE, FsFilterDispatchCreate)
//#pragma alloc_text(PAGE, CBTdPreOperationCallback)
//#endif

//
// DriverEntry
//

NTSTATUS
ProcMonDriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS Status = STATUS_SUCCESS;
    DbgPrint("ProcMon DriverEntry\n");

    g_pDriverObject = DriverObject;

    UNICODE_STRING NtDeviceName;
    UNICODE_STRING DosDevicesLinkName;

    //
    // Create our device object.
    //
    RtlInitUnicodeString(&NtDeviceName, NT_DEVICE_NAME.Buffer);
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
    RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICE_NAME.Buffer);
    Status = IoCreateSymbolicLink(&DosDevicesLinkName, &NtDeviceName);
    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice(g_pDeviceObject);
        return Status;
    }

    DriverObject->DriverUnload = ProcMonDeviceUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ProcMonDispatchPassthrough;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ProcMonDispatchPassthrough;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProcMonDeviceControl;

    DbgPrint("ProcMon DriverEntry Success\n");

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
ProcMonDeviceUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING  DosDevicesLinkName;

    RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICE_NAME.Buffer);
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
ProcMonDeviceControl(
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

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_CALLBACK_START:
        Status = IPC_Init(pioControl->Type, pioControl->CallbackPrefix);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("main, DeviceControl, IPC_Init Fail : 0x%x\n", Status);
            goto End;
        }
		switch (pioControl->Type)
		{
		case 0:
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
		case 1:
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
        case 2:
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

End:
    //
    // Assign the length of the data copied to IoStatus.Information
    // of the Irp and complete the Irp.
    //
    Irp->IoStatus.Information = sizeof(NTSTATUS);

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
ProcMonDispatchPassthrough(
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


VOID
SetDriverName(
    PWCH NT_NAME,
    PWCH DOS_NAME
)
{
    if (NT_NAME == NULL
        || DOS_NAME == NULL)
        return;

    RtlInitUnicodeString(&NT_DEVICE_NAME, NT_NAME);
    RtlInitUnicodeString(&DOS_DEVICE_NAME, DOS_NAME);
}

VOID
SetOBName(
    PWCH NT_NAME,
    PWCH DOS_NAME
)
{
    if (NT_NAME == NULL
        || DOS_NAME == NULL)
        return;

    RtlInitUnicodeString(&OB_DEVICE_NAME, NT_NAME);
    RtlInitUnicodeString(&OB_DOS_DEVICES_LINK_NAME, DOS_NAME);
}

VOID
SetFSName(
    PWCH NT_NAME,
    PWCH DOS_NAME
)
{
    if (NT_NAME == NULL
        || DOS_NAME == NULL)
        return;

    RtlInitUnicodeString(&FS_DEVICE_NAME, NT_NAME);
    RtlInitUnicodeString(&FS_DOS_DEVICES_LINK_NAME, DOS_NAME);
}

VOID
SetREGName(
    PWCH NT_NAME,
    PWCH DOS_NAME
)
{
    if (NT_NAME == NULL
        || DOS_NAME == NULL)
        return;

    RtlInitUnicodeString(&REG_DEVICE_NAME, NT_NAME);
    RtlInitUnicodeString(&REG_DOS_DEVICES_LINK_NAME, DOS_NAME);
}