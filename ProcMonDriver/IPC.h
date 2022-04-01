#pragma once
#include <ntifs.h>

#define PROC_MAX_PATH 32767

typedef struct _DATALIST
{
	PVOID Data;
	PVOID* NextData;
} DATALIST, * PDATALIST;

typedef struct _OBDATA
{
	INT64 SystemTick;	// count of 100-nanosecond intervals since January 1, 1601
	LONG PID;
	LONG TargetPID;
	OB_OPERATION Operation;
	ACCESS_MASK DesiredAccess;
} OBDATA, * POBDATA;

typedef struct _FSDATA
{
	INT64 SystemTick;
	LONG PID;
	UCHAR MajorFunction;
	WCHAR FileName[PROC_MAX_PATH];
} FSDATA, * PFSDATA;

typedef struct _REGDATA
{
	INT64 SystemTick;
	LONG PID;
	LONG NotifyClass;
	WCHAR RegistryFullPath[PROC_MAX_PATH];	//RegistryPath + '\' + RegistryName
} REGDATA, * PREGDATA;

typedef struct IPCStruct
{
	OBDATA ObData;
	FSDATA FsData;
	REGDATA RegData;
} IPCSTRUCT, * PIPCSTRUCT;

/// <summary>
/// Callback에서 들어오는 데이터 형식
/// </summary>
typedef struct _COMDATA
{
	LONG Type;
	PVOID Data;
} COMDATA, * PCOMDATA;

#define IPC_BUFFER2 sizeof(IPCSTRUCT)

typedef struct test
{
	LONG PID;
	LONG Length;
	WCHAR buffer[1024];
} test, * ptest;

NTSTATUS CreateStandardSCAndACL(
	OUT PSECURITY_DESCRIPTOR* SecurityDescriptor,
	OUT PACL* Acl
);

NTSTATUS GrantAccess(
	HANDLE hSection,
	IN PACL StandardAcl);

NTSTATUS CreateSharedMemory(
);

VOID ClearSharedMemory(
);

VOID MapViewTest(
);

VOID
CreateData(
	PVOID pData,
	LONG Type
);

VOID
DataInsertThread(
	_In_ PVOID pComData
);

VOID
POPDataThread(
	PVOID ThreadContext
);