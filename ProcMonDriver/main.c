#include "pch.h"
#include "OBCallback.h"
#include "FSFilter.h"
#include "RegFilter.h"

//////////////////////////////////////////////////////////////////////////
// Global data

PDRIVER_OBJECT   g_fsFilterDriverObject = NULL;
PDEVICE_OBJECT   g_DeviceObj = NULL;

//
// Registry callback version
//
ULONG g_MajorVersion;
ULONG g_MinorVersion;

FAST_IO_DISPATCH g_fastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),
    FsFilterFastIoCheckIfPossible,
    FsFilterFastIoRead,
    FsFilterFastIoWrite,
    FsFilterFastIoQueryBasicInfo,
    FsFilterFastIoQueryStandardInfo,
    FsFilterFastIoLock,
    FsFilterFastIoUnlockSingle,
    FsFilterFastIoUnlockAll,
    FsFilterFastIoUnlockAllByKey,
    FsFilterFastIoDeviceControl,
    NULL,
    NULL,
    FsFilterFastIoDetachDevice,
    FsFilterFastIoQueryNetworkOpenInfo,
    NULL,
    FsFilterFastIoMdlRead,
    FsFilterFastIoMdlReadComplete,
    FsFilterFastIoPrepareMdlWrite,
    FsFilterFastIoMdlWriteComplete,
    FsFilterFastIoReadCompressed,
    FsFilterFastIoWriteCompressed,
    FsFilterFastIoMdlReadCompleteCompressed,
    FsFilterFastIoMdlWriteCompleteCompressed,
    FsFilterFastIoQueryOpen,
    NULL,
    NULL,
    NULL,
};

//
// Function declarations
//
DRIVER_INITIALIZE  DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH TdDeviceCreate;
_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH TdDeviceClose;
_Dispatch_type_(IRP_MJ_CLEANUP) DRIVER_DISPATCH TdDeviceCleanup;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH TdDeviceControl;

DRIVER_UNLOAD   TdDeviceUnload;

//
// DriverEntry
//

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS Status;
    USHORT CallbackVersion;
    ULONG i = 0;

    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ObCallbackTest: DriverEntry: Driver loaded. Use ed nt!Kd_IHVDRIVER_Mask f (or 7) to enable more traces\n");

    CallbackVersion = ObGetFilterVersion();

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ObCallbackTest: DriverEntry: Callback version 0x%hx\n", CallbackVersion);

    //
    // Store our driver object.
    //

    g_fsFilterDriverObject = DriverObject;

    //
    // Initialize globals.
    //

    KeInitializeGuardedMutex(&TdCallbacksMutex);

    //
    //  Initialize the driver object dispatch table.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
    {
        DriverObject->MajorFunction[i] = FsFilterDispatchPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsFilterDispatchCreate;
    DriverObject->DriverUnload = TdDeviceUnload;


    Status = TdInitOBCallback();
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Set fast-io dispatch table.
    //

    DriverObject->FastIoDispatch = &g_fastIoDispatch;

    //
    //  Registered callback routine for file system changes.
    //

    Status = IoRegisterFsRegistrationChange(DriverObject, FsFilterNotificationCallback);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

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
        &g_DeviceObj);                // returned device object pointer

    if (!NT_SUCCESS(Status)) {
        DbgPrint("IoCreateDevice RegFilter fail : 0x%x\n", Status);
        goto Exit;
    }

    //
    // Create a link in the Win32 namespace.
    //

    RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICES_LINK_NAME);

    Status = IoCreateSymbolicLink(&DosDevicesLinkName, &NtDeviceName);

    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice(g_DeviceObj);
        DbgPrint("IoCreateSymbolicLink RegFilter fail : 0x%x\n", Status);
        goto Exit;
    }

    //
    // Get callback version.
    //

    CmGetCallbackVersion(&g_MajorVersion, &g_MinorVersion);
    DbgPrint("Callback version %u.%u\n", g_MajorVersion, g_MinorVersion);

    //
    // Initialize the callback context list
    //

    InitializeListHead(&g_CallbackCtxListHead);
    ExInitializeFastMutex(&g_CallbackCtxListLock);
    g_NumCallbackCtxListEntries = 0;

    Status = RegisterCallback(g_DeviceObj);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("RegisterCallback fail 0x%x\n", Status);
    }


Exit:

    if (!NT_SUCCESS(Status))
    {

    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ObCallbackTest: DriverEntry: End 0x%x\n", Status);

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
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG           numDevices = 0;
    ULONG           i = 0;
    LARGE_INTEGER   interval;
    PDEVICE_OBJECT  devList[DEVOBJ_LIST_SIZE];

    UNICODE_STRING  DosDevicesLinkName;

    interval.QuadPart = (1 * DELAY_ONE_SECOND); //delay 5 seconds


    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ObCallbackTest: TdDeviceUnload\n");

    //
    // remove any OB callbacks
    //
    Status = TdDeleteOBCallback();
    TD_ASSERT(Status == STATUS_SUCCESS);

    Status = UnRegisterCallback(g_DeviceObj);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("UnRegisterCallback fail : 0x%x", Status);
    }

    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //

    RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICES_LINK_NAME);
    IoDeleteSymbolicLink(&DosDevicesLinkName);

    //
    // Finally delete our device object
    //

    IoDeleteDevice(g_DeviceObj);

    //
    //  Unregistered callback routine for file system changes.
    //

    IoUnregisterFsRegistrationChange(DriverObject, FsFilterNotificationCallback);

    //
    //  This is the loop that will go through all of the devices we are attached
    //  to and detach from them.
    //

    for (;;)
    {
        IoEnumerateDeviceObjectList(
            DriverObject,
            devList,
            sizeof(devList),
            &numDevices);

        if (0 == numDevices)
        {
            break;
        }

        numDevices = min(numDevices, RTL_NUMBER_OF(devList));

        for (i = 0; i < numDevices; ++i)
        {
            FsFilterDetachFromDevice(devList[i]);
            ObDereferenceObject(devList[i]);
        }

        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }
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

//
// Function:
//
//     TdDeviceControl
//
// Description:
//
//     This function handles 'control' irp.
//

NTSTATUS
TdDeviceControl(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    return STATUS_SUCCESS;
}
