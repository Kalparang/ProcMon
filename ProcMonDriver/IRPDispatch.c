#include "FSFilter.h"
#include "IPC.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// PassThrough IRP Handler

NTSTATUS FsFilterDispatchPassThrough(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP           Irp
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    PFSDATA2 pFsData = NULL;
    size_t len = 0;
    PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION pStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT pFileObject = NULL;
    LARGE_INTEGER UTCTime;

    if (pStackLocation->MajorFunction == IRP_MJ_CREATE)
    {
        if (pStackLocation->FileObject != NULL)
        {
            pFileObject = pStackLocation->FileObject;
            KeQuerySystemTime(&UTCTime);

            //DbgPrint("FSInfo : Control : %u | Flags : %u | MajFunc : %u | MinFunc : %u | %wZ\n",
            //    pStackLocation->Control, pStackLocation->Flags, pStackLocation->MajorFunction, pStackLocation->MinorFunction, pFileObject->FileName);

            pFsData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(FSDATA2), 'fs');
            if (pFsData != NULL)
            {
                pFsData->MajorFunction = pStackLocation->MajorFunction;
                pFsData->PID = 0;
                pFsData->SystemTick = UTCTime.QuadPart;
                pFsData->FileName = NULL;

                if (pFileObject->FileName.Buffer != NULL)
                {
                    len = 1;
                    len += wcslen(pFileObject->FileName.Buffer);
                    len *= sizeof(WCHAR);

                    pFsData->FileName = ExAllocatePool2(POOL_FLAG_PAGED, len, 'fs');
                    if (pFsData->FileName != NULL)
                        wcscpy(pFsData->FileName, pFileObject->FileName.Buffer);
                    else
                        goto Exit;
                }
                CreateData(pFsData, 1);

                ntStatus = STATUS_SUCCESS;
            }
            else
                goto Exit;
        }
    }

Exit:
    if (!NT_SUCCESS(ntStatus))
    {
        if (pFsData != NULL)
        {
            if (pFsData->FileName != NULL) ExFreePool(pFsData->FileName);
            ExFreePool(pFsData);
            DbgPrint("IRPDispatch, FsFilterDispatchPassThrough fail : 0x%x\n", ntStatus);
        }
    }

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