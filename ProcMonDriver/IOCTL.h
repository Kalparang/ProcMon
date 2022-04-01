//#pragma once
//
//#define COM_OB 0
//#define COM_FS 1
//#define COM_REG 2
//#define IPC_BUFFER 1024
//
//typedef struct _DATALIST
//{
//	PVOID Data;
//	PVOID* NextData;
//} DATALIST, * PDATALIST;
//
//typedef struct _OBDATA
//{
//	INT64 SystemTick;	// count of 100-nanosecond intervals since January 1, 1601
//	LONG PID;
//	LONG TargetPID;
//	OB_OPERATION Operation;
//	ACCESS_MASK DesiredAccess;
//} OBDATA, * POBDATA;
//
//typedef struct _FSDATA
//{
//	INT64 SystemTick;
//	LONG PID;
//	UCHAR MajorFunction;
//	PWCH FileName;
//} FSDATA, * PFSDATA;
//
//typedef struct _REGDATA
//{
//	INT64 SystemTick;
//	LONG PID;
//	LONG NotifyClass;
//	PWCH RegistryFullPath;	//RegistryPath + '\' + RegistryName
//} REGDATA, * PREGDATA;
//
///// <summary>
///// Callback에서 들어오는 데이터 형식
///// </summary>
//typedef struct _COMDATA
//{
//	LONG Type;
//	PVOID Data;
//} COMDATA, * PCOMDATA;
//
//#define FSSIZE sizeof(FSDATA)
//#define FSSTRINGSIZE IPC_BUFFER - FSSIZE - sizeof(LONG)
//#define REGSIZE sizeof(REGDATA)
//#define REGSTRINGSIZE (IPC_BUFFER - REGSIZE - sizeof(size_t)) / 2
//
//typedef struct _FSOUTDATA
//{
//	LONG FilePathLength;
//	PFSDATA FsData;
//	WCHAR FilePath[FSSTRINGSIZE];
//} FSOUTDAT, * PFSOUTDATA;
//
//typedef struct _REGOUTDATA
//{
//	size_t RegistryPathLength;
//	REGDATA RegData;
//	WCHAR RegistryPath[REGSTRINGSIZE];
//} REGOUTDATA, * PREGOUTDATA;
//
//VOID
//IOUnload(
//);
//
//VOID
//POPData(
//	ULONG Type,
//	PVOID OutData
//);
//
//VOID
//DataInsertThread(
//	_In_ PVOID pComData
//);
//
//VOID
//CreateData(
//	PVOID pData,
//	LONG Type
//);
//
//VOID
//IOInit(
//);
//
//NTSTATUS
//ioctlDeviceControl(
//	PDEVICE_OBJECT DeviceObject,
//	PIRP Irp
//);