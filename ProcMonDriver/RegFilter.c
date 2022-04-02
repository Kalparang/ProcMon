/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    regfltr.c

Abstract:

    Sample driver used to run the kernel mode registry callback samples.

Environment:

    Kernel mode only

--*/

#include "RegFilter.h"
#include "common.h"
#include "IPC.h"

//
// The root key used in the samples
//
LARGE_INTEGER CallbackCtxCookie;

FAST_MUTEX g_CallbackCtxListLock;
LIST_ENTRY g_CallbackCtxListHead;
USHORT g_NumCallbackCtxListEntries;

PDEVICE_OBJECT   g_RegDeviceObject = NULL;
BOOLEAN g_RegSymbolicLink = FALSE;

//
// Registry callback version
//
ULONG g_MajorVersion;
ULONG g_MinorVersion;

LPCWSTR
GetNotifyClassString(
    _In_ REG_NOTIFY_CLASS NotifyClass
);

NTSTATUS
RegFilterInit(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING NtDeviceName;
    UNICODE_STRING DosDevicesLinkName;

    //
    // Create our device object.
    //
    RtlInitUnicodeString(&NtDeviceName, REG_DEVICE_NAME);
    Status = IoCreateDevice(
        DriverObject,                 // pointer to driver object
        0,                            // device extension size
        &NtDeviceName,                // device name
        FILE_DEVICE_UNKNOWN,          // device type
        0,                            // device characteristics
        FALSE,                         // not exclusive
        &g_RegDeviceObject);                // returned device object pointer

    if (!NT_SUCCESS(Status)) {
        RegFilterUnload(DriverObject);
        return Status;
    }

    //
    // Create a link in the Win32 namespace.
    //
    RtlInitUnicodeString(&DosDevicesLinkName, REG_DOS_DEVICES_LINK_NAME);
    Status = IoCreateSymbolicLink(&DosDevicesLinkName, &NtDeviceName);
    if (!NT_SUCCESS(Status)) {
        RegFilterUnload(DriverObject);
        return Status;
    }

    g_RegSymbolicLink = TRUE;

    //
    // Get callback version.
    //
    CmGetCallbackVersion(&g_MajorVersion, &g_MinorVersion);

    //
    // Initialize the callback context list
    //
    InitializeListHead(&g_CallbackCtxListHead);
    ExInitializeFastMutex(&g_CallbackCtxListLock);
    g_NumCallbackCtxListEntries = 0;

    Status = RegisterCallback(g_RegDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        RegFilterUnload(DriverObject);
        return Status;
    }

    return Status;
}

NTSTATUS
RegFilterUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING  DosDevicesLinkName;

    if (g_RegDeviceObject != NULL)
    {
        Status = UnRegisterCallback(g_RegDeviceObject);
    }

    if (g_RegSymbolicLink)
    {
        RtlInitUnicodeString(&DosDevicesLinkName, REG_DOS_DEVICES_LINK_NAME);
        IoDeleteSymbolicLink(&DosDevicesLinkName);
    }

    if (g_RegDeviceObject != NULL)
    {
        IoDeleteDevice(g_RegDeviceObject);
        g_RegDeviceObject = NULL;
    }

    return Status;
}

NTSTATUS
Callback(
    _In_     PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
)
/*++

Routine Description:

    This is the registry callback we'll register to intercept all registry
    operations.

Arguments:

    CallbackContext - The value that the driver passed to the Context parameter
        of CmRegisterCallbackEx when it registers this callback routine.

    Argument1 - A REG_NOTIFY_CLASS typed value that identifies the type of
        registry operation that is being performed and whether the callback
        is being called in the pre or post phase of processing.

    Argument2 - A pointer to a structure that contains information specific
        to the type of the registry operation. The structure type depends
        on the REG_NOTIFY_CLASS value of Argument1. Refer to MSDN for the
        mapping from REG_NOTIFY_CLASS to REG_XXX_KEY_INFORMATION.

Return Value:

    Status returned from the helper callback routine or STATUS_SUCCESS if
    the registry operation did not originate from this process.

--*/
{
    REG_NOTIFY_CLASS NotifyClass;
    PCALLBACK_CONTEXT CallbackCtx;

    CallbackCtx = (PCALLBACK_CONTEXT)CallbackContext;
    NotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

    if (CallbackCtx->ProcessId == PsGetCurrentProcessId()) {
        return STATUS_SUCCESS;
    }

    //
    // Invoke a helper method depending on the value of CallbackMode in 
    // CallbackCtx.
    //

    if (Argument2 == NULL) {

        //
        // This should never happen but the sal annotation on the callback 
        // function marks Argument 2 as opt and is looser than what 
        // it actually is.
        //
        //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
        //    "Callback Argument2 is NULL\n");

        return STATUS_SUCCESS;
    }

    CallbackMonitor(NotifyClass, Argument2);

    return STATUS_SUCCESS;
}

NTSTATUS
RegisterCallback(
    _In_ PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    Registers a callback with the specified callback mode and altitude

Arguments:

    DeviceObject - The device object receiving the request.

    Irp - The request packet.

Return Value:

    Status from CmRegisterCallbackEx

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCALLBACK_CONTEXT CallbackCtx = NULL;

    //
    // Create the callback context from the specified callback mode and altitude
    //

    CallbackCtx = CreateCallbackContext(L"380010");

    if (CallbackCtx == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // Register the callback
    //

    Status = CmRegisterCallbackEx(Callback,
        &CallbackCtx->Altitude,
        DeviceObject->DriverObject,
        (PVOID)CallbackCtx,
        &CallbackCtx->Cookie,
        NULL);
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "CmRegisterCallback failed. Status 0x%x\n", Status);
        goto Exit;
    }

    CallbackCtxCookie = CallbackCtx->Cookie;

    if (!InsertCallbackContext(CallbackCtx)) {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

Exit:
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "RegisterCallback failed. Status 0x%x\n", Status);
        if (CallbackCtx != NULL) {
            DeleteCallbackContext(CallbackCtx);
        }
    }
    else {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "RegisterCallback succeeded\n");
    }

    return Status;
}



NTSTATUS
UnRegisterCallback(
    _In_ PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    Unregisters a callback with the specified cookie and clean up the
    callback context.

Arguments:

    DeviceObject - The device object receiving the request.

    Irp - The request packet.

Return Value:

    Status from CmUnRegisterCallback

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCALLBACK_CONTEXT CallbackCtx;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Unregister the callback with the cookie
    //

    Status = CmUnRegisterCallback(CallbackCtxCookie);

    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "CmUnRegisterCallback failed. Status 0x%x\n", Status);
        goto Exit;
    }

    //
    // Free the callback context buffer
    //
    CallbackCtx = CreateCallbackContext(L"380010");
    if (CallbackCtx != NULL) {
        DeleteCallbackContext(CallbackCtx);
    }

Exit:

    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "UnRegisterCallback failed. Status 0x%x\n", Status);
    }
    else {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "UnRegisterCallback succeeded\n");
    }

    return Status;
}


NTSTATUS
GetCallbackVersion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
)
/*++

Routine Description:

    Calls CmGetCallbackVersion

Arguments:

    DeviceObject - The device object receiving the request.

    Irp - The request packet.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpStack;
    ULONG OutputBufferLength;
    PGET_CALLBACK_VERSION_OUTPUT GetCallbackVersionOutput;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Get the output buffer and verify its size
    //

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (OutputBufferLength < sizeof(GET_CALLBACK_VERSION_OUTPUT)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    GetCallbackVersionOutput = (PGET_CALLBACK_VERSION_OUTPUT)Irp->AssociatedIrp.SystemBuffer;

    //
    // Call CmGetCallbackVersion and store the results in the output buffer
    //

    CmGetCallbackVersion(&GetCallbackVersionOutput->MajorVersion,
        &GetCallbackVersionOutput->MinorVersion);

    Irp->IoStatus.Information = sizeof(GET_CALLBACK_VERSION_OUTPUT);

Exit:

    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "GetCallbackVersion failed. Status 0x%x\n", Status);
    }
    else {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
            "GetCallbackVersion succeeded\n");
    }

    return Status;
}


LPCWSTR
GetNotifyClassString(
    _In_ REG_NOTIFY_CLASS NotifyClass
)
/*++

Routine Description:

    Converts from NotifyClass to a string

Arguments:

    NotifyClass - value that identifies the type of registry operation that
        is being performed

Return Value:

    Returns a string of the name of NotifyClass.

--*/
{
    switch (NotifyClass) {
    case RegNtPreDeleteKey:                 return L"RegNtPreDeleteKey";
    case RegNtPreSetValueKey:               return L"RegNtPreSetValueKey";
    case RegNtPreDeleteValueKey:            return L"RegNtPreDeleteValueKey";
    case RegNtPreSetInformationKey:         return L"RegNtPreSetInformationKey";
    case RegNtPreRenameKey:                 return L"RegNtPreRenameKey";
    case RegNtPreEnumerateKey:              return L"RegNtPreEnumerateKey";
    case RegNtPreEnumerateValueKey:         return L"RegNtPreEnumerateValueKey";
    case RegNtPreQueryKey:                  return L"RegNtPreQueryKey";
    case RegNtPreQueryValueKey:             return L"RegNtPreQueryValueKey";
    case RegNtPreQueryMultipleValueKey:     return L"RegNtPreQueryMultipleValueKey";
    case RegNtPreKeyHandleClose:            return L"RegNtPreKeyHandleClose";
    case RegNtPreCreateKeyEx:               return L"RegNtPreCreateKeyEx";
    case RegNtPreOpenKeyEx:                 return L"RegNtPreOpenKeyEx";
    case RegNtPreFlushKey:                  return L"RegNtPreFlushKey";
    case RegNtPreLoadKey:                   return L"RegNtPreLoadKey";
    case RegNtPreUnLoadKey:                 return L"RegNtPreUnLoadKey";
    case RegNtPreQueryKeySecurity:          return L"RegNtPreQueryKeySecurity";
    case RegNtPreSetKeySecurity:            return L"RegNtPreSetKeySecurity";
    case RegNtPreRestoreKey:                return L"RegNtPreRestoreKey";
    case RegNtPreSaveKey:                   return L"RegNtPreSaveKey";
    case RegNtPreReplaceKey:                return L"RegNtPreReplaceKey";

    case RegNtPostDeleteKey:                return L"RegNtPostDeleteKey";
    case RegNtPostSetValueKey:              return L"RegNtPostSetValueKey";
    case RegNtPostDeleteValueKey:           return L"RegNtPostDeleteValueKey";
    case RegNtPostSetInformationKey:        return L"RegNtPostSetInformationKey";
    case RegNtPostRenameKey:                return L"RegNtPostRenameKey";
    case RegNtPostEnumerateKey:             return L"RegNtPostEnumerateKey";
    case RegNtPostEnumerateValueKey:        return L"RegNtPostEnumerateValueKey";
    case RegNtPostQueryKey:                 return L"RegNtPostQueryKey";
    case RegNtPostQueryValueKey:            return L"RegNtPostQueryValueKey";
    case RegNtPostQueryMultipleValueKey:    return L"RegNtPostQueryMultipleValueKey";
    case RegNtPostKeyHandleClose:           return L"RegNtPostKeyHandleClose";
    case RegNtPostCreateKeyEx:              return L"RegNtPostCreateKeyEx";
    case RegNtPostOpenKeyEx:                return L"RegNtPostOpenKeyEx";
    case RegNtPostFlushKey:                 return L"RegNtPostFlushKey";
    case RegNtPostLoadKey:                  return L"RegNtPostLoadKey";
    case RegNtPostUnLoadKey:                return L"RegNtPostUnLoadKey";
    case RegNtPostQueryKeySecurity:         return L"RegNtPostQueryKeySecurity";
    case RegNtPostSetKeySecurity:           return L"RegNtPostSetKeySecurity";
    case RegNtPostRestoreKey:               return L"RegNtPostRestoreKey";
    case RegNtPostSaveKey:                  return L"RegNtPostSaveKey";
    case RegNtPostReplaceKey:               return L"RegNtPostReplaceKey";

    case RegNtCallbackObjectContextCleanup: return L"RegNtCallbackObjectContextCleanup";

    default:
        return L"Unsupported REG_NOTIFY_CLASS";
    }
}

VOID
CallbackMonitor(
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
)
/*++

Routine Description:

    This helper callback routine just monitors how many pre and post registry
    operations it receives and records it in the callback context.

Arguments:

    CallbackContext - The value that the driver passed to the Context parameter
        of CmRegisterCallbackEx when it registers this callback routine.

    NotifyClass - A REG_NOTIFY_CLASS typed value that identifies the type of
        registry operation that is being performed and whether the callback
        is being called in the pre or post phase of processing.

    Argument2 - A pointer to a structure that contains information specific
        to the type of the registry operation. The structure type depends
        on the REG_NOTIFY_CLASS value of Argument1.

Return Value:

    Always STATUS_SUCCESS

--*/
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;
    POBJECT_NAME_INFORMATION RegistryPath;
    ULONG nReturnBytes;
    PVOID RegistryObject = NULL;
    UNICODE_STRING RegistryName = { 0 };
    UNICODE_STRING NotifyClassString = { 0 };

    RtlInitUnicodeString(&RegistryName, L"");
    RtlInitUnicodeString(&NotifyClassString, GetNotifyClassString(NotifyClass));

    switch (NotifyClass) {
    case RegNtPreDeleteKey:
    {
        PREG_DELETE_KEY_INFORMATION PreDeleteKey =
            (PREG_DELETE_KEY_INFORMATION)Argument2;

        if (PreDeleteKey->Object != NULL)
            RegistryObject = PreDeleteKey->Object;

        //RtlInitUnicodeString(&RegistryName, L"");
    }
    break;
    case RegNtPreSetValueKey:
    {
        PREG_SET_VALUE_KEY_INFORMATION PreSetValueKey =
            (PREG_SET_VALUE_KEY_INFORMATION)Argument2;

        if (PreSetValueKey->Object != NULL)
            RegistryObject = PreSetValueKey->Object;

        if (PreSetValueKey->ValueName != NULL)
            if (PreSetValueKey->ValueName->Buffer != NULL)
                RtlInitUnicodeString(&RegistryName, PreSetValueKey->ValueName->Buffer);
    }
    break;
    case RegNtPreDeleteValueKey:
    {
        PREG_DELETE_VALUE_KEY_INFORMATION PreDeleteValueKey =
            (PREG_DELETE_VALUE_KEY_INFORMATION)Argument2;

        if (PreDeleteValueKey->Object != NULL)
            RegistryObject = PreDeleteValueKey->Object;

        if (PreDeleteValueKey->ValueName != NULL)
            if (PreDeleteValueKey->ValueName->Buffer != NULL)
                RtlInitUnicodeString(&RegistryName, PreDeleteValueKey->ValueName->Buffer);
    }
    break;
    //case RegNtPreSetInformationKey:
    case RegNtPreRenameKey:
    {
        PREG_RENAME_KEY_INFORMATION PreRenameKey =
            (PREG_RENAME_KEY_INFORMATION)Argument2;

        if (PreRenameKey->Object != NULL)
            RegistryObject = PreRenameKey->Object;

        //if (PreRenameKey->NewName != NULL)
        //    if (PreRenameKey->NewName->Buffer != NULL)
        //        RtlInitUnicodeString(&RegistryName, PreRenameKey->NewName->Buffer);
    }
    break;
    //case RegNtPreEnumerateKey:
    //case RegNtPreEnumerateValueKey:
    case RegNtPreQueryKey:
    {
        PREG_QUERY_KEY_INFORMATION PreQueryKey =
            (PREG_QUERY_KEY_INFORMATION)Argument2;

        if (PreQueryKey->Object != NULL)
            RegistryObject = PreQueryKey->Object;

        //RtlInitUnicodeString(&RegistryName, L"");
    }
    break;
    case RegNtPreQueryValueKey:
    {
        PREG_QUERY_VALUE_KEY_INFORMATION PreQueryValueKey =
            (PREG_QUERY_VALUE_KEY_INFORMATION)Argument2;

        if (PreQueryValueKey->Object != NULL)
            RegistryObject = PreQueryValueKey->Object;

        if (PreQueryValueKey->ValueName != NULL)
            if (PreQueryValueKey->ValueName->Buffer != NULL)
                RtlInitUnicodeString(&RegistryName, PreQueryValueKey->ValueName->Buffer);
    }
    break;
    //case RegNtPreQueryMultipleValueKey:
    case RegNtPreKeyHandleClose:
    {
        PREG_KEY_HANDLE_CLOSE_INFORMATION PreKeyHandleClose =
            (PREG_KEY_HANDLE_CLOSE_INFORMATION)Argument2;

        if (PreKeyHandleClose->Object != NULL)
            RegistryObject = PreKeyHandleClose->Object;

        //RtlInitUnicodeString(&RegistryName, L"");
    }
    break;
    case RegNtPreCreateKeyEx:
    {
        PREG_CREATE_KEY_INFORMATION_V1 PreCreateKeyV1 =
            (PREG_CREATE_KEY_INFORMATION_V1)Argument2;

        if (PreCreateKeyV1->RootObject != NULL)
            RegistryObject = PreCreateKeyV1->RootObject;

        if (PreCreateKeyV1->CompleteName != NULL)
            if (PreCreateKeyV1->CompleteName->Buffer != NULL)
                RtlInitUnicodeString(&RegistryName, PreCreateKeyV1->CompleteName->Buffer);
    }
    break;
    case RegNtPreOpenKeyEx:
    {
        PREG_OPEN_KEY_INFORMATION_V1 PreOpenKeyV1 =
            (PREG_OPEN_KEY_INFORMATION_V1)Argument2;

        if (PreOpenKeyV1->RootObject != NULL)
            RegistryObject = PreOpenKeyV1->RootObject;

        if (PreOpenKeyV1->CompleteName != NULL)
            if (PreOpenKeyV1->CompleteName->Buffer != NULL)
                RtlInitUnicodeString(&RegistryName, PreOpenKeyV1->CompleteName->Buffer);
    }
    break;
    //case RegNtPreFlushKey:
    //case RegNtPreLoadKey:
    //case RegNtPreUnLoadKey:
    //case RegNtPreQueryKeySecurity:
    //case RegNtPreSetKeySecurity:
    //case RegNtPreRestoreKey:
    //case RegNtPreSaveKey:
    //case RegNtPreReplaceKey:
        //InterlockedIncrement(&CallbackCtx->PreNotificationCount);
        //break;
    //case RegNtPostDeleteKey:
    //case RegNtPostSetValueKey:
    //case RegNtPostDeleteValueKey:
    //case RegNtPostSetInformationKey:
    //case RegNtPostRenameKey:
    //case RegNtPostEnumerateKey:
    //case RegNtPostEnumerateValueKey:
    //case RegNtPostQueryKey:
    //case RegNtPostQueryValueKey:
    //case RegNtPostQueryMultipleValueKey:
    //case RegNtPostKeyHandleClose:
    //case RegNtPostCreateKeyEx:
    //case RegNtPostOpenKeyEx:
    //case RegNtPostFlushKey:
    //case RegNtPostLoadKey:
    //case RegNtPostUnLoadKey:
    //case RegNtPostQueryKeySecurity:
    //case RegNtPostSetKeySecurity:
    //case RegNtPostRestoreKey:
    //case RegNtPostSaveKey:
    //case RegNtPostReplaceKey:
    //    InterlockedIncrement(&CallbackCtx->PostNotificationCount);
    //    break;
    default:
        break;
    }

    if (RegistryObject == NULL)
    {
        return;
    }
    
    Status = ObQueryNameString(RegistryObject,
        NULL,
        0,
        &nReturnBytes);
    if (Status == STATUS_INFO_LENGTH_MISMATCH
        && nReturnBytes > 0)
    {
        RegistryPath = ExAllocatePool2(POOL_FLAG_PAGED, nReturnBytes + sizeof(WCHAR), 'reg');
        if (RegistryPath == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status = ObQueryNameString(RegistryObject,
                RegistryPath,
                nReturnBytes,
                &nReturnBytes);

            if (NT_SUCCESS(Status))
            {
                //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
                //    "RegFilterInfo : %wZ | %wZ\\%wZ\n", NotifyClassString, RegistryPath->Name, RegistryName);
                
                LARGE_INTEGER UTCTime;
                KeQuerySystemTime(&UTCTime);
                PREGDATA pRegData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(REGDATA), 'reg');
                if (pRegData == NULL)
                {
                    DbgPrint("RegFilter,ExAllocatePool2 pRegData insufficient memory\n");
                }
                else
                {
                    pRegData->NotifyClass = NotifyClass;
                    //pRegData->PID = PsGetCurrentProcessId();
                    pRegData->PID = 0;
                    wcscpy(pRegData->RegistryFullPath, RegistryPath->Name.Buffer);
                    wcscat(pRegData->RegistryFullPath, L"\\");
                    wcscat(pRegData->RegistryFullPath, RegistryName.Buffer);
                    pRegData->SystemTick = UTCTime.QuadPart;
                    //CreateData(pRegData, 2);
                    ExFreePool(pRegData);
                }
            }

            ExFreePool(RegistryPath);
        }
    }
    if (!NT_SUCCESS(Status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "ObQueryNameString fail : 0x%x\n", Status);
    }

    return;
}

NTSTATUS
CallbackSetObjectContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
)
/*++

Routine Description:

    This helper callback routine shows how to associate a registry key object
    with context information using CmSetCallbackObjectContext. The context
    set is then only available to this callback. A callback that sets the
    object context should be prepared for a RegNtCallbackObjectContextCleanup
    where it must clean up the context.

Arguments:

    CallbackContext - The value that the driver passed to the Context parameter
        of CmRegisterCallbackEx when it registers this callback routine.

    NotifyClass - A REG_NOTIFY_CLASS typed value that identifies the type of
        registry operation that is being performed and whether the callback
        is being called in the pre or post phase of processing.

    Argument2 - A pointer to a structure that contains information specific
        to the type of the registry operation. The structure type depends
        on the REG_NOTIFY_CLASS value of Argument1.

Return Value:

    Always STATUS_SUCCESS;

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PREG_CALLBACK_CONTEXT_CLEANUP_INFORMATION CleanupInfo;
    PREG_POST_OPERATION_INFORMATION PostInfo;
    PVOID ObjectContext = NULL;

    switch (NotifyClass) {

    case RegNtPostOpenKeyEx:

        PostInfo = (PREG_POST_OPERATION_INFORMATION)Argument2;

        //
        // If the open key was successful, set an object context
        // to the key object. 
        //
        // Note that one of the parameters of CmSetCallbackObjectContext
        // is the cookie gotten from registering a callback. The object
        // context will only be available to the callback with that
        // particular cookie. 
        //

        if (NT_SUCCESS(PostInfo->Status)) {

            //
            // Never call CmSetCallbackObjectContext outside of the
            // callback routine.
            //

            Status = CmSetCallbackObjectContext(PostInfo->Object,
                &CallbackCtx->Cookie,
                CallbackCtx,
                NULL);

            if (!NT_SUCCESS(Status)) {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                    "CmSetCallbackobjectContext failed.Status 0x%x\n", Status);
            }
        }
        break;

    case RegNtPreSetValueKey:
    case RegNtPostSetValueKey:

        //
        // All registry operations using the handle received from the open
        // key operation will come with the ObjectContext field set to the
        // context information. Other operations on the same key but 
        // using a different handle will not have the ObjectContext field 
        // set.
        //

        if (NotifyClass == RegNtPreSetValueKey) {
            ObjectContext = ((PREG_SET_VALUE_KEY_INFORMATION)Argument2)->ObjectContext;
        }
        else {
            ObjectContext = ((PREG_POST_OPERATION_INFORMATION)Argument2)->ObjectContext;
        }

        if (ObjectContext == NULL) {
            InterlockedIncrement(&CallbackCtx->NotificationWithNoContextCount);
        }
        else if (ObjectContext == CallbackCtx) {
            InterlockedIncrement(&CallbackCtx->NotificationWithContextCount);
        }
        else {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                "Unexpected ObjectContext value: 0x%p\n", ObjectContext);
        }

        break;

    case RegNtCallbackObjectContextCleanup:

        //
        // This is a special notification only invoked for callbacks
        // that have set context information to an object. This notification 
        // is either sent when the registry object is being closed or if
        // the callback is being unregistered. In the first case, this 
        // notification comes after the RegNtPreKeyHandleClose
        // notification and before the RegNtPostKeyHandleClose notification.
        //

        CleanupInfo = (PREG_CALLBACK_CONTEXT_CLEANUP_INFORMATION)Argument2;
        if (CleanupInfo->ObjectContext != CallbackCtx) {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                "ContextCleanup's ObjectContext has unexpected value: 0x%p.\n",
                CleanupInfo->ObjectContext);
        }
        else {
            InterlockedIncrement(&CallbackCtx->ContextCleanupCount);
        }
        break;

    default:
        //
        // Do nothing for other notifications
        //
        break;
    }

    return Status;
}

PVOID
CreateCallbackContext(
    _In_ PCWSTR AltitudeString
)
/*++

Routine Description:

    Utility method to create a callback context. Callback context
    should be freed using DeleteCallbackContext.

Arguments:

    CallbackMode - the callback mode value

    AltitudeString - a string with the altitude the callback will be
        registered at

Return Value:

    Pointer to the allocated and initialized callback context

--*/
{

    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    BOOLEAN Success = FALSE;

    CallbackCtx = (PCALLBACK_CONTEXT)ExAllocatePoolZero(
        PagedPool,
        sizeof(CALLBACK_CONTEXT),
        REGFLTR_CONTEXT_POOL_TAG);

    if (CallbackCtx == NULL) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "CreateCallbackContext failed due to insufficient resources.\n");
        goto Exit;
    }

    CallbackCtx->ProcessId = PsGetCurrentProcessId();

    Status = RtlStringCbPrintfW(CallbackCtx->AltitudeBuffer,
        MAX_ALTITUDE_BUFFER_LENGTH * sizeof(WCHAR),
        L"%s",
        AltitudeString);

    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "RtlStringCbPrintfW in CreateCallbackContext failed. Status 0x%x\n", Status);
        goto Exit;
    }

    RtlInitUnicodeString(&CallbackCtx->Altitude, CallbackCtx->AltitudeBuffer);

    Success = TRUE;

Exit:

    if (Success == FALSE) {
        if (CallbackCtx != NULL) {
            ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
            CallbackCtx = NULL;
        }
    }

    return CallbackCtx;
}

BOOLEAN
InsertCallbackContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx
)
/*++

Routine Description:

    Utility method to insert the callback context into a list.

Arguments:

    CallbackCtx - the callback context to insert

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{

    BOOLEAN Success = FALSE;

    ExAcquireFastMutex(&g_CallbackCtxListLock);

    if (g_NumCallbackCtxListEntries < MAX_CALLBACK_CTX_ENTRIES) {
        g_NumCallbackCtxListEntries++;
        InsertHeadList(&g_CallbackCtxListHead, &CallbackCtx->CallbackCtxList);
        Success = TRUE;
    }
    else {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "Insert Callback Ctx failed : Max CallbackCtx entries reached.\n");
    }

    ExReleaseFastMutex(&g_CallbackCtxListLock);

    return Success;

}

PCALLBACK_CONTEXT
FindCallbackContext(
    _In_ LARGE_INTEGER Cookie
)
/*++

Routine Description:

    Utility method to find a callback context using the cookie value.

Arguments:

    Cookie - the cookie value associated with the callback context. The
             cookie is returned when CmRegisterCallbackEx is called.

Return Value:

    Pointer to the found callback context

--*/
{

    PCALLBACK_CONTEXT CallbackCtx = NULL;
    PLIST_ENTRY Entry;

    ExAcquireFastMutex(&g_CallbackCtxListLock);

    Entry = g_CallbackCtxListHead.Flink;
    while (Entry != &g_CallbackCtxListHead) {

        CallbackCtx = CONTAINING_RECORD(Entry,
            CALLBACK_CONTEXT,
            CallbackCtxList);
        if (CallbackCtx->Cookie.QuadPart == Cookie.QuadPart) {
            break;
        }

        Entry = Entry->Flink;
    }

    ExReleaseFastMutex(&g_CallbackCtxListLock);

    if (CallbackCtx == NULL) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL,
            "FindCallbackContext failed: No context with specified cookied was found.\n");
    }

    return CallbackCtx;

}

PCALLBACK_CONTEXT
FindAndRemoveCallbackContext(
    _In_ LARGE_INTEGER Cookie
)
/*++

Routine Description:

    Utility method to find a callback context using the cookie value and then
    remove it.

Arguments:

    Cookie - the cookie value associated with the callback context. The
             cookie is returned when CmRegisterCallbackEx is called.

Return Value:

    Pointer to the found callback context

--*/
{

    PCALLBACK_CONTEXT CallbackCtx = NULL;
    PLIST_ENTRY Entry;

    ExAcquireFastMutex(&g_CallbackCtxListLock);

    Entry = g_CallbackCtxListHead.Flink;
    while (Entry != &g_CallbackCtxListHead) {

        CallbackCtx = CONTAINING_RECORD(Entry,
            CALLBACK_CONTEXT,
            CallbackCtxList);
        if (CallbackCtx->Cookie.QuadPart == Cookie.QuadPart) {
            RemoveEntryList(&CallbackCtx->CallbackCtxList);
            g_NumCallbackCtxListEntries--;
            break;
        }
    }

    ExReleaseFastMutex(&g_CallbackCtxListLock);

    if (CallbackCtx == NULL) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL,
            "FindAndRemoveCallbackContext failed: No context with specified cookied was found.\n");
    }

    return CallbackCtx;
}

VOID
DeleteCallbackContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx
)
/*++

Routine Description:

    Utility method to delete a callback context.

Arguments:

    CallbackCtx - the callback context to insert

Return Value:

    None

--*/
{

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

}
