#include <ntddk.h>
#include <ntstrsafe.h>

/*++

Module Name:

    tdriver.c

Abstract:

    Main module for the Ob and Ps sample code

Notice:
    Use this sample code at your own risk; there is no support from Microsoft for the sample code.
    In addition, this sample code is licensed to you under the terms of the Microsoft Public License
    (http://www.microsoft.com/opensource/licenses.mspx)


--*/

#include "ProcessWatcher.h"

//
// Globals
//

KGUARDED_MUTEX TdCallbacksMutex;
BOOLEAN bCallbacksInstalled = FALSE;


#define CB_PROCESS_TERMINATE 0x0001
#define CB_THREAD_TERMINATE  0x0001

//  The following are for setting up callbacks for Process and Thread filtering
PVOID pCBRegistrationHandle = NULL;

OB_CALLBACK_REGISTRATION  CBObRegistration = { 0 };
OB_OPERATION_REGISTRATION CBOperationRegistrations[2] = { { 0 }, { 0 } };
UNICODE_STRING CBAltitude = { 0 };
TD_CALLBACK_REGISTRATION CBCallbackRegistration = { 0 };

// Here is the protected process
WCHAR   TdwProtectName[NAME_SIZE + 1] = { 0 };
PVOID   TdProtectedTargetProcess = NULL;
HANDLE  TdProtectedTargetProcessId = { 0 };

//
// Function declarations
//
//DRIVER_INITIALIZE  DriverEntry;

//_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH TdDeviceCreate;
//_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH TdDeviceClose;
//_Dispatch_type_(IRP_MJ_CLEANUP) DRIVER_DISPATCH TdDeviceCleanup;
//_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH TdDeviceControl;

//DRIVER_UNLOAD   TdDeviceUnload;

VOID
TdCreateProcessNotifyRoutine2(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (CreateInfo != NULL)
    {

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "ObCallbackTest: TdCreateProcessNotifyRoutine2: process %p (ID 0x%p) created, creator %Ix:%Ix\n"
            "    command line %wZ\n"
            "    file name %wZ (FileOpenNameAvailable: %d)\n",
            Process,
            (PVOID)ProcessId,
            (ULONG_PTR)CreateInfo->CreatingThreadId.UniqueProcess,
            (ULONG_PTR)CreateInfo->CreatingThreadId.UniqueThread,
            CreateInfo->CommandLine,
            CreateInfo->ImageFileName,
            CreateInfo->FileOpenNameAvailable
        );

        // Search for matching process to protect only if filtering
        if (TdbProtectName) {
            if (CreateInfo->CommandLine != NULL)
            {
                Status = TdCheckProcessMatch(CreateInfo->CommandLine, Process, ProcessId);

                if (Status == STATUS_SUCCESS) {
                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ObCallbackTest: TdCreateProcessNotifyRoutine2: PROTECTING process %p (ID 0x%p)\n",
                        Process,
                        (PVOID)ProcessId
                    );
                }
            }

        }

        // Search for matching process to reject process creation
        if (TdbRejectName) {
            if (CreateInfo->CommandLine != NULL)
            {
                Status = TdCheckProcessMatch(CreateInfo->CommandLine, Process, ProcessId);

                if (Status == STATUS_SUCCESS) {
                    DbgPrintEx(
                        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ObCallbackTest: TdCreateProcessNotifyRoutine2: REJECTING process %p (ID 0x%p)\n",
                        Process,
                        (PVOID)ProcessId
                    );

                    CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
                }
            }

        }
    }
    else
    {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ObCallbackTest: TdCreateProcessNotifyRoutine2: process %p (ID 0x%p) destroyed\n",
            Process,
            (PVOID)ProcessId
        );
    }
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

//VOID
//TdDeviceUnload(
//    _In_ PDRIVER_OBJECT DriverObject
//)
//{
//    NTSTATUS Status = STATUS_SUCCESS;
//    UNICODE_STRING DosDevicesLinkName = RTL_CONSTANT_STRING(TD_DOS_DEVICES_LINK_NAME);
//
//    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ObCallbackTest: TdDeviceUnload\n");
//
//    //
//    // Unregister process notify routines.
//    //
//
//    //if (TdProcessNotifyRoutineSet2 == TRUE)
//    //{
//    //    Status = PsSetCreateProcessNotifyRoutineEx(
//    //        (PCREATE_PROCESS_NOTIFY_ROUTINE_EX)TdCreateProcessNotifyRoutine2,
//    //        TRUE
//    //    );
//
//    //    TD_ASSERT(Status == STATUS_SUCCESS);
//
//    //    TdProcessNotifyRoutineSet2 = FALSE;
//    //}
//
//    // remove filtering and remove any OB callbacks
//    TdbProtectName = FALSE;
//    Status = TdDeleteProtectNameCallback();
//    TD_ASSERT(Status == STATUS_SUCCESS);
//
//    //
//    // Delete the link from our device name to a name in the Win32 namespace.
//    //
//
//    Status = IoDeleteSymbolicLink(&DosDevicesLinkName);
//    if (Status != STATUS_INSUFFICIENT_RESOURCES) {
//        //
//        // IoDeleteSymbolicLink can fail with STATUS_INSUFFICIENT_RESOURCES.
//        //
//
//        TD_ASSERT(NT_SUCCESS(Status));
//
//    }
//
//
//    //
//    // Delete our device object.
//    //
//
//    IoDeleteDevice(DriverObject->DeviceObject);
//}

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
// TdControlProtectName
//

NTSTATUS TdControlProtectName(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpStack = NULL;
    ULONG InputBufferLength = 0;
    PTD_PROTECTNAME_INPUT pProtectNameInput = NULL;

    UNREFERENCED_PARAMETER(DeviceObject);


    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TdControlProtectName: Entering\n");

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    if (InputBufferLength < sizeof(TD_PROTECTNAME_INPUT))
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    pProtectNameInput = (PTD_PROTECTNAME_INPUT)Irp->AssociatedIrp.SystemBuffer;

    Status = TdProtectNameCallback();

    switch (pProtectNameInput->Operation) {
    case TDProtectName_Protect:
        // Begin filtering access rights
        TdbProtectName = TRUE;
        TdbRejectName = FALSE;
        break;

    case TDProtectName_Reject:
        // Begin reject process creation on match
        TdbProtectName = FALSE;
        TdbRejectName = TRUE;
        break;
    }


Exit:
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TD_IOCTL_PROTECTNAME: Status %x\n", Status);

    return Status;
}

//
// TdControlUnprotect
//

NTSTATUS TdControlUnprotect(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    // PIO_STACK_LOCATION IrpStack = NULL;
    // ULONG InputBufferLength = 0;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    // IrpStack = IoGetCurrentIrpStackLocation (Irp);
    // InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    // No need to check length of passed in parameters as we do not need any information from that

    // do not filter requested access
    Status = TdDeleteProtectNameCallback();
    if (Status != STATUS_SUCCESS) {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "ObCallbackTest: TdDeleteProtectNameCallback:  status 0x%x\n", Status);
    }
    TdbProtectName = FALSE;
    TdbRejectName = FALSE;

    //Exit:
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TD_IOCTL_UNPROTECT: exiting - status 0x%x\n", Status);

    return Status;
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
    PIO_STACK_LOCATION IrpStack;
    ULONG Ioctl;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);


    Status = STATUS_SUCCESS;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Ioctl = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "TdDeviceControl: entering - ioctl code 0x%x\n", Ioctl);

    switch (Ioctl)
    {
    case TD_IOCTL_PROTECT_NAME_CALLBACK:

        Status = TdControlProtectName(DeviceObject, Irp);
        break;

    case TD_IOCTL_UNPROTECT_CALLBACK:

        Status = TdControlUnprotect(DeviceObject, Irp);
        break;


    default:
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "TdDeviceControl: unrecognized ioctl code 0x%x\n", Ioctl);
        break;
    }

    //
    // Complete the irp and return.
    //

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "TdDeviceControl leaving - status 0x%x\n", Status);
    return Status;
}

//
// TdSetCallContext
//
// Creates a call context object and stores a pointer to it
// in the supplied OB_PRE_OPERATION_INFORMATION structure.
//
// This function is called from a pre-notification. The created call context
// object then has to be freed in a corresponding post-notification using
// TdCheckAndFreeCallContext.
//

void TdSetCallContext(
    _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo,
    _In_ PTD_CALLBACK_REGISTRATION CallbackRegistration
)
{
    PTD_CALL_CONTEXT CallContext;

    CallContext = (PTD_CALL_CONTEXT)ExAllocatePoolWithTag(
        PagedPool, sizeof(TD_CALL_CONTEXT), TD_CALL_CONTEXT_TAG
    );

    if (CallContext == NULL)
    {
        return;
    }

    RtlZeroMemory(CallContext, sizeof(TD_CALL_CONTEXT));

    CallContext->CallbackRegistration = CallbackRegistration;
    CallContext->Operation = PreInfo->Operation;
    CallContext->Object = PreInfo->Object;
    CallContext->ObjectType = PreInfo->ObjectType;

    PreInfo->CallContext = CallContext;
}

void TdCheckAndFreeCallContext(
    _Inout_ POB_POST_OPERATION_INFORMATION PostInfo,
    _In_ PTD_CALLBACK_REGISTRATION CallbackRegistration
)
{
    PTD_CALL_CONTEXT CallContext = (PTD_CALL_CONTEXT)PostInfo->CallContext;

    if (CallContext != NULL)
    {
        TD_ASSERT(CallContext->CallbackRegistration == CallbackRegistration);

        TD_ASSERT(CallContext->Operation == PostInfo->Operation);
        TD_ASSERT(CallContext->Object == PostInfo->Object);
        TD_ASSERT(CallContext->ObjectType == PostInfo->ObjectType);

        ExFreePoolWithTag(CallContext, TD_CALL_CONTEXT_TAG);
    }
}


//
// TdDeleteProtectNameCallback
//
NTSTATUS TdDeleteProtectNameCallback()
{
    NTSTATUS Status = STATUS_SUCCESS;

    drv_debug_print(DPFLTR_TRACE_LEVEL, __FUNCTION__,
        "TdDeleteProtectNameCallback entering");

    KeAcquireGuardedMutex(&TdCallbacksMutex);

    // if the callbacks are active - remove them
    if (bCallbacksInstalled == TRUE) {
        ObUnRegisterCallbacks(pCBRegistrationHandle);
        pCBRegistrationHandle = NULL;
        bCallbacksInstalled = FALSE;
    }


    KeReleaseGuardedMutex(&TdCallbacksMutex);

    drv_debug_print(DPFLTR_TRACE_LEVEL, __FUNCTION__,
        " TdDeleteProtectNameCallback exiting  - status 0x%x",
        Status);

    return Status;
}


//
// TdProtectNameCallback
//

NTSTATUS TdProtectNameCallback(
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    drv_debug_print(DPFLTR_TRACE_LEVEL, __FUNCTION__,
        "TdProtectNameCallback entering");

    KeAcquireGuardedMutex(&TdCallbacksMutex);

    // Need to enable the OB callbacks
    // once the process is matched to a newly created process, the callbacks will protect the process
    if (bCallbacksInstalled == FALSE) {
        // Setup the Ob Registration calls

        CBOperationRegistrations[0].ObjectType = PsProcessType;
        CBOperationRegistrations[0].Operations |= OB_OPERATION_HANDLE_CREATE;
        CBOperationRegistrations[0].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
        CBOperationRegistrations[0].PreOperation = CBTdPreOperationCallback;
        CBOperationRegistrations[0].PostOperation = CBTdPostOperationCallback;

        //CBOperationRegistrations[1].ObjectType = PsThreadType;
        //CBOperationRegistrations[1].Operations |= OB_OPERATION_HANDLE_CREATE;
        //CBOperationRegistrations[1].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
        //CBOperationRegistrations[1].PreOperation = CBTdPreOperationCallback;
        //CBOperationRegistrations[1].PostOperation = CBTdPostOperationCallback;


        RtlInitUnicodeString(&CBAltitude, L"1000");

        CBObRegistration.Version = OB_FLT_REGISTRATION_VERSION;
        CBObRegistration.OperationRegistrationCount = 2;
        //CBObRegistration.OperationRegistrationCount = 1;
        CBObRegistration.Altitude = CBAltitude;
        CBObRegistration.RegistrationContext = &CBCallbackRegistration;
        CBObRegistration.OperationRegistration = CBOperationRegistrations;


        Status = ObRegisterCallbacks(
            &CBObRegistration,
            &pCBRegistrationHandle       // save the registration handle to remove callbacks later
        );

        if (!NT_SUCCESS(Status)) {
            drv_debug_print(DPFLTR_ERROR_LEVEL, __FUNCTION__,
                "ObCallbackTest: installing OB callbacks failed  status 0x%x",
                Status);

            KeReleaseGuardedMutex(&TdCallbacksMutex); // Release the lock before exit
            goto Exit;
        }
        bCallbacksInstalled = TRUE;

    }


    KeReleaseGuardedMutex(&TdCallbacksMutex);

Exit:
    drv_debug_print(DPFLTR_TRACE_LEVEL, __FUNCTION__,
        "ObCallbackTest: TdProtectNameCallback: exiting  status 0x%x",
        Status);
    return Status;
}


//
// TdCheckProcessMatch - function to test a command line to see if the process is to be protected
//
NTSTATUS TdCheckProcessMatch(
    _In_ PCUNICODE_STRING pustrCommand,
    _In_ PEPROCESS Process,
    _In_ HANDLE ProcessId
)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    WCHAR   CommandLineBuffer[NAME_SIZE + 1] = { 0 };    // force a NULL termination
    USHORT  CommandLineBytes = 0;

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TdCheckProcessMatch: entering\n");

    if (!pustrCommand || !pustrCommand->Buffer) {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "ObCallbackTest: TdCheckProcessMatch: no Command line provided\n"
        );
        Status = FALSE;
        goto Exit;
    }
    else {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "ObCallbackTest: TdCheckProcessMatch:              checking for %ls\n", TdwProtectName
        );
    }

    KeAcquireGuardedMutex(&TdCallbacksMutex);


    // Make sure that the CommandLineBuffer is NULL terminated
    if (pustrCommand->Length < (NAME_SIZE * sizeof(WCHAR)))
        CommandLineBytes = pustrCommand->Length;
    else
        CommandLineBytes = NAME_SIZE * sizeof(WCHAR);

    if (CommandLineBytes) {
        memcpy(CommandLineBuffer, pustrCommand->Buffer, CommandLineBytes);

        // now check if the process to protect is in the command line

        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "ObCallbackTest: TdCheckProcessMatch: command line %ls\n", CommandLineBuffer
        );

        if (NULL != wcsstr(CommandLineBuffer, TdwProtectName)) {
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
                "ObCallbackTest: TdCheckProcessMatch: match FOUND\n"
            );

            // Set the process to watch
            TdProtectedTargetProcess = Process;
            TdProtectedTargetProcessId = ProcessId;

            Status = STATUS_SUCCESS;
        }
    }
    else {
        Status = FALSE;     // no command line buffer provided
    }

    KeReleaseGuardedMutex(&TdCallbacksMutex);


Exit:

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TdCheckProcessMatch: leaving    status  0x%x\n", Status
    );
    return Status;
}


//
// CBTdPreOperationCallback
//
OB_PREOP_CALLBACK_STATUS
CBTdPreOperationCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo
)
{
    PTD_CALLBACK_REGISTRATION CallbackRegistration;

    ACCESS_MASK AccessBitsToClear = 0;
    ACCESS_MASK AccessBitsToSet = 0;
    ACCESS_MASK InitialDesiredAccess = 0;
    ACCESS_MASK OriginalDesiredAccess = 0;


    PACCESS_MASK DesiredAccess = NULL;

    LPCWSTR ObjectTypeName = NULL;
    LPCWSTR OperationName = NULL;

    // Not using driver specific values at this time
    CallbackRegistration = (PTD_CALLBACK_REGISTRATION)RegistrationContext;


    TD_ASSERT(PreInfo->CallContext == NULL);

    // Only want to filter attempts to access protected process
    // all other processes are left untouched

    if (PreInfo->ObjectType == *PsProcessType) {
        //
        // Ignore requests for processes other than our target process.
        //

        //
        // Also ignore requests that are trying to open/duplicate the current
        // process.
        //

        if (PreInfo->Object == PsGetCurrentProcess()) {
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
                "ObCallbackTest: CBTdPreOperationCallback: ignore process open/duplicate from the protected process itself\n");
            goto Exit;
        }

        ObjectTypeName = L"PsProcessType";
        AccessBitsToClear = CB_PROCESS_TERMINATE;
        AccessBitsToSet = 0;
    }
    //else if (PreInfo->ObjectType == *PsThreadType) {
    //    HANDLE ProcessIdOfTargetThread = PsGetThreadProcessId((PETHREAD)PreInfo->Object);

    //    //
    //    // Ignore requests for threads belonging to processes other than our
    //    // target process.
    //    //

    //    // if (CallbackRegistration->TargetProcess   != NULL &&
    //    //     CallbackRegistration->TargetProcessId != ProcessIdOfTargetThread)
    //    if (TdProtectedTargetProcessId != ProcessIdOfTargetThread) {
    //        goto Exit;
    //    }

    //    //
    //    // Also ignore requests for threads belonging to the current processes.
    //    //

    //    if (ProcessIdOfTargetThread == PsGetCurrentProcessId()) {
    //        DbgPrintEx(
    //            DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
    //            "ObCallbackTest: CBTdPreOperationCallback: ignore thread open/duplicate from the protected process itself\n");
    //        goto Exit;
    //    }

    //    ObjectTypeName = L"PsThreadType";
    //    AccessBitsToClear = CB_THREAD_TERMINATE;
    //    AccessBitsToSet = 0;
    //}
    else {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "ObCallbackTest: CBTdPreOperationCallback: unexpected object type\n");
        goto Exit;
    }

    switch (PreInfo->Operation) {
    case OB_OPERATION_HANDLE_CREATE:
        DesiredAccess = &PreInfo->Parameters->CreateHandleInformation.DesiredAccess;
        OriginalDesiredAccess = PreInfo->Parameters->CreateHandleInformation.OriginalDesiredAccess;

        OperationName = L"OB_OPERATION_HANDLE_CREATE";
        break;

    case OB_OPERATION_HANDLE_DUPLICATE:
        DesiredAccess = &PreInfo->Parameters->DuplicateHandleInformation.DesiredAccess;
        OriginalDesiredAccess = PreInfo->Parameters->DuplicateHandleInformation.OriginalDesiredAccess;

        OperationName = L"OB_OPERATION_HANDLE_DUPLICATE";
        break;

    default:
        TD_ASSERT(FALSE);
        break;
    }

    //
    // Set call context.
    //

    //TdSetCallContext(PreInfo, CallbackRegistration);

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: CBTdPreOperationCallback\n"
        "    Client Id:    %p:%p\n"
        "    Object:       %p\n"
        "    Type:         %ls\n"
        "    Operation:    %ls (KernelHandle=%d)\n"
        "    OriginalDesiredAccess: 0x%x\n"
        "    DesiredAccess :    0x%x\n",
        PsGetCurrentProcessId(),
        PsGetCurrentThreadId(),
        PreInfo->Object,
        ObjectTypeName,
        OperationName,
        PreInfo->KernelHandle,
        OriginalDesiredAccess,
        InitialDesiredAccess
    );

Exit:

    return OB_PREOP_SUCCESS;
}

//
// TdPostOperationCallback
//

VOID
CBTdPostOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_POST_OPERATION_INFORMATION PostInfo
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    UNREFERENCED_PARAMETER(PostInfo);
    return;

    //PTD_CALLBACK_REGISTRATION CallbackRegistration = (PTD_CALLBACK_REGISTRATION)RegistrationContext;

    ////TdCheckAndFreeCallContext(PostInfo, CallbackRegistration);

    //if (PostInfo->ObjectType == *PsProcessType) {
    //    //
    //    // Ignore requests for processes other than our target process.
    //    //

    //    if (CallbackRegistration->TargetProcess != NULL &&
    //        CallbackRegistration->TargetProcess != PostInfo->Object
    //        ) {
    //        return;
    //    }

    //    //
    //    // Also ignore requests that are trying to open/duplicate the current
    //    // process.
    //    //

    //    if (PostInfo->Object == PsGetCurrentProcess()) {
    //        return;
    //    }
    //}
    //else if (PostInfo->ObjectType == *PsThreadType) {
    //    HANDLE ProcessIdOfTargetThread = PsGetThreadProcessId((PETHREAD)(PostInfo->Object));

    //    //
    //    // Ignore requests for threads belonging to processes other than our
    //    // target process.
    //    //

    //    if (CallbackRegistration->TargetProcess != NULL &&
    //        CallbackRegistration->TargetProcessId != ProcessIdOfTargetThread
    //        ) {
    //        return;
    //    }

    //    //
    //    // Also ignore requests for threads belonging to the current processes.
    //    //

    //    if (ProcessIdOfTargetThread == PsGetCurrentProcessId()) {
    //        return;
    //    }
    //}
    //else {
    //    TD_ASSERT(FALSE);
    //}

}

