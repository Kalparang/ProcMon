#include "FSFilter.h"
#include "IPC.h"

extern BOOLEAN g_bFsCallBack;

///////////////////////////////////////////////////////////////////////////////////////////////////
// PassThrough IRP Handler

NTSTATUS FsFilterDispatchPassThrough(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP           Irp
)
{
    PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
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
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    PFSDATA2 pFsData = NULL;
    size_t len = 0;
    PIO_STACK_LOCATION pStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT pFileObject = NULL;
    LARGE_INTEGER UTCTime;

    //if (pStackLocation->MajorFunction == IRP_MJ_CREATE)
    {
        if (pStackLocation->FileObject != NULL)
        {
            pFileObject = pStackLocation->FileObject;
            KeQuerySystemTime(&UTCTime);

            //DbgPrint("FSInfo : Control : %u | Flags : %u | MajFunc : %u | MinFunc : %u | %wZ\n",
            //    pStackLocation->Control, pStackLocation->Flags, pStackLocation->MajorFunction, pStackLocation->MinorFunction, pFileObject->FileName);
            if (g_bFsCallBack)
            {
                pFsData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(FSDATA2), 'fs');
                if (pFsData != NULL)
                {
                    pFsData->MajorFunction = pStackLocation->MajorFunction;
                    pFsData->PID = PsGetCurrentProcessId();
                    pFsData->SystemTick = UTCTime.QuadPart;
                    pFsData->Flag = pStackLocation->Flags;
                    pFsData->FileName = NULL;

                    if (pFileObject->FileName.Length > 0)
                    {
                        len = 1;
                        len += pFileObject->FileName.Length;
                        len *= sizeof(WCHAR);

                        pFsData->FileName = ExAllocatePool2(POOL_FLAG_PAGED, len, 'fs');
                        if (pFsData->FileName != NULL)
							wcsncpy(pFsData->FileName, pFileObject->FileName.Buffer, pFileObject->FileName.Length);
                        else
                            goto Exit;
                    }
                    CreateData(pFsData, 1);

                    ntStatus = STATUS_SUCCESS;
                }
                else
                    goto Exit;
            }
            else
                ntStatus = STATUS_SUCCESS;
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

    return FsFilterDispatchPassThrough(DeviceObject, Irp);
}