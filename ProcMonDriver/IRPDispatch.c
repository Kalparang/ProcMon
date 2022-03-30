#include "FSFilter.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// PassThrough IRP Handler

NTSTATUS FsFilterDispatchPassThrough(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP           Irp
)
{
    PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION pStackLocation = IoGetCurrentIrpStackLocation(Irp);
    //UCHAR control = pStackLocation->Control;
    //UCHAR flags = pStackLocation->Flags;
    //UCHAR MajorFunction = pStackLocation->MajorFunction;
    //UCHAR MinorFunction = pStackLocation->MinorFunction;
    PFILE_OBJECT pFileObject = NULL;
    
    if(pStackLocation->FileObject != NULL)
        pFileObject = pStackLocation->FileObject;

    //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
    //    "FSInfo : Control : %u | Flags : %u | MajFunc : %u | MinFunc : %u | %wZ\n",
    //    control, flags, MajorFunction, MinorFunction, pFileObject->FileName);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(pDevExt->AttachedToDeviceObject, Irp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// IRP_MJ_CREATE IRP Handler

NTSTATUS FsFilterDispatchCreate(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP           Irp
)
{
    //PIO_STACK_LOCATION pStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //PFILE_OBJECT pFileObject = pStackLocation->FileObject;

    return FsFilterDispatchPassThrough(DeviceObject, Irp);
}