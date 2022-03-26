#include "pch.h"
#include "OBCallback.h"

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

//
// TdDeleteProtectNameCallback
//
NTSTATUS TdDeleteOBCallback()
{
    NTSTATUS Status = STATUS_SUCCESS;

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TdDeleteProtectNameCallback entering\n");

    KeAcquireGuardedMutex(&TdCallbacksMutex);

    // if the callbacks are active - remove them
    if (bCallbacksInstalled == TRUE) {
        ObUnRegisterCallbacks(pCBRegistrationHandle);
        pCBRegistrationHandle = NULL;
        bCallbacksInstalled = FALSE;
    }


    KeReleaseGuardedMutex(&TdCallbacksMutex);

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TdDeleteProtectNameCallback exiting  - status 0x%x\n", Status
    );

    return Status;
}

//
// TdProtectNameCallback
//

NTSTATUS TdInitOBCallback(
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    KeAcquireGuardedMutex(&TdCallbacksMutex);

    // Need to enable the OB callbacks
    // once the process is matched to a newly created process, the callbacks will protect the process
    if (bCallbacksInstalled == FALSE) {
        DbgPrintEx(
            DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "ObCallbackTest: TdProtectNameCallback: installing callbacks\n"
        );

        // Setup the Ob Registration calls

        CBOperationRegistrations[0].ObjectType = PsProcessType;
        CBOperationRegistrations[0].Operations |= OB_OPERATION_HANDLE_CREATE;
        CBOperationRegistrations[0].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
        CBOperationRegistrations[0].PreOperation = CBTdPreOperationCallback;
        CBOperationRegistrations[0].PostOperation = CBTdPostOperationCallback;

        CBOperationRegistrations[1].ObjectType = PsThreadType;
        CBOperationRegistrations[1].Operations |= OB_OPERATION_HANDLE_CREATE;
        CBOperationRegistrations[1].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
        CBOperationRegistrations[1].PreOperation = CBTdPreOperationCallback;
        CBOperationRegistrations[1].PostOperation = CBTdPostOperationCallback;


        RtlInitUnicodeString(&CBAltitude, L"1000");

        CBObRegistration.Version = OB_FLT_REGISTRATION_VERSION;
        CBObRegistration.OperationRegistrationCount = 2;
        CBObRegistration.Altitude = CBAltitude;
        CBObRegistration.RegistrationContext = &CBCallbackRegistration;
        CBObRegistration.OperationRegistration = CBOperationRegistrations;


        Status = ObRegisterCallbacks(
            &CBObRegistration,
            &pCBRegistrationHandle       // save the registration handle to remove callbacks later
        );

        if (!NT_SUCCESS(Status)) {
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                "ObCallbackTest: installing OB callbacks failed  status 0x%x\n", Status
            );
            KeReleaseGuardedMutex(&TdCallbacksMutex); // Release the lock before exit
            goto Exit;
        }
        bCallbacksInstalled = TRUE;

    }


    KeReleaseGuardedMutex(&TdCallbacksMutex);

Exit:
    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: TdProtectNameCallback: exiting  status 0x%x\n", Status
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
        // Also ignore requests that are trying to open/duplicate the current
        // process.
        //

        if (PreInfo->Object == PsGetCurrentProcess()) {
        //    DbgPrintEx(
        //        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        //        "ObCallbackTest: CBTdPreOperationCallback: ignore process open/duplicate from the protected process itself\n");
            goto Exit;
        }

        ObjectTypeName = L"PsProcessType";
        AccessBitsToClear = CB_PROCESS_TERMINATE;
        AccessBitsToSet = 0;
    }
    else if (PreInfo->ObjectType == *PsThreadType) {
        goto Exit;
        //HANDLE ProcessIdOfTargetThread = PsGetThreadProcessId((PETHREAD)PreInfo->Object);
        ////
        //// Also ignore requests for threads belonging to the current processes.
        ////

        //if (ProcessIdOfTargetThread == PsGetCurrentProcessId()) {
        ////    DbgPrintEx(
        ////        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        ////        "ObCallbackTest: CBTdPreOperationCallback: ignore thread open/duplicate from the protected process itself\n");
        //    goto Exit;
        //}

        //ObjectTypeName = L"PsThreadType";
        //AccessBitsToClear = CB_THREAD_TERMINATE;
        //AccessBitsToSet = 0;
    }
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

    InitialDesiredAccess = *DesiredAccess;

    DbgPrintEx(
        DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
        "ObCallbackTest: CBTdPreOperationCallback\n"
        "    Client Id:    %p:%p\n"
        "    Object:       %p\n"
        "    Type:         %ls\n"
        "    Operation:    %ls (KernelHandle=%d)\n"
        "    OriginalDesiredAccess: 0x%x\n"
        "    DesiredAccess:    0x%x\n",
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
}

