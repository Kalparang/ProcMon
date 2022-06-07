#pragma once
#include <ntifs.h>

#define PROC_MAX_PATH 32767

typedef struct _LISTDATA
{
	PVOID Data;
	PVOID NextData;
} LISTDATA, * PLISTDATA;

typedef struct _OBDATA
{
	INT64 SystemTick;	// count of 100-nanosecond intervals since January 1, 1601
	HANDLE PID;
	HANDLE TargetPID;
	OB_OPERATION Operation;
	ACCESS_MASK DesiredAccess;
} OBDATA, * POBDATA;

typedef struct _FSDATA
{
	INT64 SystemTick;
	HANDLE PID;
	UCHAR MajorFunction;
	BOOLEAN DeletePending;
	BOOLEAN DeleteAccess;
	WCHAR FileName[PROC_MAX_PATH];
} FSDATA, * PFSDATA;

typedef struct _FSDATA2
{
	INT64 SystemTick;
	HANDLE PID;
	UCHAR MajorFunction;
	BOOLEAN DeletePending;
	BOOLEAN DeleteAccess;
	PWCH FileName;
} FSDATA2, * PFSDATA2;

typedef struct _REGDATA
{
	INT64 SystemTick;
	HANDLE PID;
	LONG NotifyClass;
	WCHAR RegistryFullPath[PROC_MAX_PATH];	//RegistryPath + '\' + RegistryName
} REGDATA, * PREGDATA;

typedef struct _REGDATA2
{
	INT64 SystemTick;
	HANDLE PID;
	LONG NotifyClass;
	WCHAR* RegistryFullPath;	//RegistryPath + '\' + RegistryName
} REGDATA2, * PREGDATA2;

/// <summary>
/// Callback에서 들어오는 데이터 형식
/// </summary>
typedef struct _COMDATA
{
	LONG Type;
	PVOID Data;
} COMDATA, * PCOMDATA;

typedef struct _POPThreadData
{
	LONG Type;
	WCHAR Prefix[32];
} POPThreadData, * pPOPThreadData;

NTSTATUS
IPC_Init(
	LONG Type,
	PWCH Prefix
);

VOID
CreateData(
	PVOID pData,
	LONG Type
);

VOID
POPDataThread(
	PVOID ThreadContext
);
