#pragma once
#include "common.h"

#define TD_CALLBACK_REGISTRATION_TAG  '0bCO' // TD_CALLBACK_REGISTRATION structure.
#define TD_CALL_CONTEXT_TAG           '1bCO' // TD_CALL_CONTEXT structure.

//
// TD_CALLBACK_REGISTRATION
//

typedef struct _TD_CALLBACK_REGISTRATION {

    //
    // Handle returned by ObRegisterCallbacks.
    //

    PVOID RegistrationHandle;

    //
    // If not NULL, filter only requests to open/duplicate handles to this
    // process (or one of its threads).
    //

    PVOID TargetProcess;
    HANDLE TargetProcessId;

    ULONG RegistrationId;        // Index in the global TdCallbacks array.

}
TD_CALLBACK_REGISTRATION, * PTD_CALLBACK_REGISTRATION;

//
// TD_CALL_CONTEXT
//

typedef struct _TD_CALL_CONTEXT
{
    PTD_CALLBACK_REGISTRATION CallbackRegistration;

    OB_OPERATION Operation;
    PVOID Object;
    POBJECT_TYPE ObjectType;
}
TD_CALL_CONTEXT, * PTD_CALL_CONTEXT;

extern KGUARDED_MUTEX TdCallbacksMutex;

// delete the process/thead OB callbacks
NTSTATUS TdDeleteOBCallback();

NTSTATUS TdInitOBCallback(
);

OB_PREOP_CALLBACK_STATUS
CBTdPreOperationCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo
);

VOID
CBTdPostOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_POST_OPERATION_INFORMATION PostInfo
);
