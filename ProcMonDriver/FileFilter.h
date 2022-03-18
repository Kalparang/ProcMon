#pragma once
/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    scrubber.h

Abstract:
    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel.

Environment:

    Kernel mode

--*/
#ifndef __SCANNER_H__
#define __SCANNER_H__

///////////////////////////////////////////////////////////////////////////
//
//  Global variables
//
///////////////////////////////////////////////////////////////////////////


typedef struct _SCANNER_DATA {

    //
    //  The object that identifies this driver.
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  The filter handle that results from a call to
    //  FltRegisterFilter.
    //

    PFLT_FILTER Filter;

    //
    //  Listens for incoming connections
    //

    PFLT_PORT ServerPort;

    //
    //  User process that connected to the port
    //

    PEPROCESS UserProcess;

    //
    //  Client port for a connection to user-mode
    //

    PFLT_PORT ClientPort;

} SCANNER_DATA, * PSCANNER_DATA;

extern SCANNER_DATA ScannerData;

typedef struct _SCANNER_STREAM_HANDLE_CONTEXT {

    BOOLEAN RescanRequired;

} SCANNER_STREAM_HANDLE_CONTEXT, * PSCANNER_STREAM_HANDLE_CONTEXT;

#pragma warning(push)
#pragma warning(disable:4200) // disable warnings for structures with zero length arrays.

typedef struct _SCANNER_CREATE_PARAMS {

    WCHAR String[0];

} SCANNER_CREATE_PARAMS, * PSCANNER_CREATE_PARAMS;

#pragma warning(pop)


///////////////////////////////////////////////////////////////////////////
//
//  Prototypes for the startup and unload routines used for 
//  this Filter.
//
//  Implementation in scanner.c
//
///////////////////////////////////////////////////////////////////////////

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

FLT_PREOP_CALLBACK_STATUS
ScannerPreCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

EXTERN_C_END

NTSTATUS
ScannerUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
ScannerQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_POSTOP_CALLBACK_STATUS
ScannerPostCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
ScannerPreCleanup(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

FLT_PREOP_CALLBACK_STATUS
ScannerPreWrite(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

#if (WINVER >= 0x0602)

FLT_PREOP_CALLBACK_STATUS
ScannerPreFileSystemControl(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

#endif

//
//  Function prototypes
//

EXTERN_C_START

NTSTATUS
ScannerInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

NTSTATUS
ScannerInitializeScannedExtensions(
    _In_ PUNICODE_STRING RegistryPath
);

VOID
ScannerFreeExtensions(
);

NTSTATUS
ScannerAllocateUnicodeString(
    _Inout_ PUNICODE_STRING String
);

VOID
ScannerFreeUnicodeString(
    _Inout_ PUNICODE_STRING String
);

NTSTATUS
ScannerPortConnect(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID* ConnectionCookie
);

VOID
ScannerPortDisconnect(
    _In_opt_ PVOID ConnectionCookie
);

EXTERN_C_END

NTSTATUS
ScannerpScanFileInUserMode(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_ PBOOLEAN SafeToOpen
);

BOOLEAN
ScannerpCheckExtension(
    _In_ PUNICODE_STRING Extension
);

const PWSTR ScannerPortName = L"\\ScannerPort";


#define SCANNER_READ_BUFFER_SIZE   1024

typedef struct _SCANNER_NOTIFICATION {

    ULONG BytesToScan;
    ULONG Reserved;             // for quad-word alignement of the Contents structure
    UCHAR Contents[SCANNER_READ_BUFFER_SIZE];

} SCANNER_NOTIFICATION, * PSCANNER_NOTIFICATION;

typedef struct _SCANNER_REPLY {

    BOOLEAN SafeToOpen;

} SCANNER_REPLY, * PSCANNER_REPLY;

#endif /* __SCANNER_H__ */

