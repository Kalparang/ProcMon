#include "FSFilter.h"

PDRIVER_OBJECT   g_FsDriverObject = NULL;

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

NTSTATUS
FsFilterInit(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    g_FsDriverObject = DriverObject;

    //
    // Initialize the driver object dispatch table.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
    {
        DriverObject->MajorFunction[i] = FsFilterDispatchPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsFilterDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsFilterDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = FsFilterDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = FsFilterDispatchCreate;

    //
    // Set fast-io dispatch table.
    //

    DriverObject->FastIoDispatch = &g_fastIoDispatch;

    //
    //  Registered callback routine for file system changes.
    //

    Status = IoRegisterFsRegistrationChange(DriverObject, FsFilterNotificationCallback);

    return Status;
}

NTSTATUS
FsFilterUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG numDevices = 0;
    ULONG i = 0;
    LARGE_INTEGER interval;
    PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];

    interval.QuadPart = (1 * DELAY_ONE_SECOND); //delay 5 seconds

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

    return Status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// This will attach to a DeviceObject that represents a mounted volume

NTSTATUS FsFilterAttachToDevice(
    __in PDEVICE_OBJECT         DeviceObject,
    __out_opt PDEVICE_OBJECT* pFilterDeviceObject
)
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PDEVICE_OBJECT              filterDeviceObject = NULL;
    PFSFILTER_DEVICE_EXTENSION  pDevExt = NULL;
    ULONG                       i = 0;

    ASSERT(!FsFilterIsAttachedToDevice(DeviceObject));

    //
    //  Create a new device object we can attach with.
    //

    status = IoCreateDevice(
        g_FsDriverObject,
        sizeof(FSFILTER_DEVICE_EXTENSION),
        NULL,
        DeviceObject->DeviceType,
        0,
        FALSE,
        &filterDeviceObject);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    pDevExt = (PFSFILTER_DEVICE_EXTENSION)filterDeviceObject->DeviceExtension;

    //
    //  Propagate flags from Device Object we are trying to attach to.
    //

    if (FlagOn(DeviceObject->Flags, DO_BUFFERED_IO))
    {
        SetFlag(filterDeviceObject->Flags, DO_BUFFERED_IO);
    }

    if (FlagOn(DeviceObject->Flags, DO_DIRECT_IO))
    {
        SetFlag(filterDeviceObject->Flags, DO_DIRECT_IO);
    }

    if (FlagOn(DeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN))
    {
        SetFlag(filterDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN);
    }

    //
    //  Do the attachment.
    //
    //  It is possible for this attachment request to fail because this device
    //  object has not finished initializing.  This can occur if this filter
    //  loaded just as this volume was being mounted.
    //

    for (i = 0; i < 8; ++i)
    {
        LARGE_INTEGER interval;

        status = IoAttachDeviceToDeviceStackSafe(
            filterDeviceObject,
            DeviceObject,
            &pDevExt->AttachedToDeviceObject);

        if (NT_SUCCESS(status))
        {
            break;
        }

        //
        //  Delay, giving the device object a chance to finish its
        //  initialization so we can try again.
        //

        interval.QuadPart = (500 * DELAY_ONE_MILLISECOND);
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    if (!NT_SUCCESS(status))
    {
        //
        // Clean up.
        //

        IoDeleteDevice(filterDeviceObject);
        filterDeviceObject = NULL;
    }
    else
    {
        //
        // Mark we are done initializing.
        //

        ClearFlag(filterDeviceObject->Flags, DO_DEVICE_INITIALIZING);

        if (NULL != pFilterDeviceObject)
        {
            *pFilterDeviceObject = filterDeviceObject;
        }
    }

    return status;
}

void FsFilterDetachFromDevice(
    __in PDEVICE_OBJECT DeviceObject
)
{
    PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IoDetachDevice(pDevExt->AttachedToDeviceObject);
    IoDeleteDevice(DeviceObject);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// This determines whether we are attached to the given device

BOOLEAN FsFilterIsAttachedToDevice(
    __in PDEVICE_OBJECT DeviceObject
)
{
    PDEVICE_OBJECT nextDevObj = NULL;
    PDEVICE_OBJECT currentDevObj = IoGetAttachedDeviceReference(DeviceObject);

    //
    //  Scan down the list to find our device object.
    //

    do
    {
        if (FsFilterIsMyDeviceObject(currentDevObj))
        {
            ObDereferenceObject(currentDevObj);
            return TRUE;
        }

        //
        //  Get the next attached object.
        //

        nextDevObj = IoGetLowerDeviceObject(currentDevObj);

        //
        //  Dereference our current device object, before moving to the next one.
        //

        ObDereferenceObject(currentDevObj);
        currentDevObj = nextDevObj;
    } while (NULL != currentDevObj);

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// Misc

BOOLEAN FsFilterIsMyDeviceObject(
    __in PDEVICE_OBJECT DeviceObject
)
{
    return DeviceObject->DriverObject == g_FsDriverObject;
}