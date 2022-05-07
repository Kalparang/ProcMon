#include <ntifs.h>
#include "IPC.h"

// https://github.com/mic101/windows/blob/master/WRK-v1.2/base/ntos/rtl/sysvol.c

#define PreShareMemory L"\\BaseNamedObjects\\"
#define SharedSectionName L"ProcSharedMemory"
#define ObKernelEventName L"ProcObKernelEvent"
#define ObUserEventName L"ProcObUserEvent"
#define FsKernelEventName L"ProcFsKernelEvent"
#define FsUserEventName L"ProcFsUserEvent"
#define RegKernelEventName L"RegKernelEvent"
#define RegUserEventName L"RegUserEvent"

static KGUARDED_MUTEX ObMutex;
static KGUARDED_MUTEX FsMutex;
static KGUARDED_MUTEX RegMutex;

static PLISTDATA g_pObList[2];
static PLISTDATA g_pFsList[2];
static PLISTDATA g_pRegList[2];

extern PDRIVER_OBJECT g_pDriverObject;
extern BOOLEAN g_bExit;
extern BOOLEAN g_bObCallBack;
extern BOOLEAN g_bFsCallBack;
extern BOOLEAN g_bRegCallBack;

VOID
POPDataThread(
	PVOID ThreadContext
);

VOID IPC_Unload(
	LONG Type
)
{
	DbgPrint("IPC_Unload Type : %ld\n", Type);

	switch (Type)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	default:
		return;
	}
}

NTSTATUS
OpenSharedMemory(
	PWCH Prefix,
	PHANDLE pSection,
	PHANDLE pSharedSection,
	HANDLE hEvent[]
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING uName = { 0 };
	OBJECT_ATTRIBUTES objAttributes = { 0 };
	SIZE_T ulViewSize = 0;
	LARGE_INTEGER lMaxSize = { 0 };
	WCHAR SharedMemoryName[1024] = { L'\0', };
	WCHAR KernelEventName[1024] = { L'\0', };
	WCHAR UserEventName[1024] = { L'\0', };

	wcscpy(SharedMemoryName, PreShareMemory);
	wcscat(SharedMemoryName, Prefix);
	wcscat(SharedMemoryName, L"SharedMemory");

	wcscpy(KernelEventName, PreShareMemory);
	wcscat(KernelEventName, Prefix);
	wcscat(KernelEventName, L"KernelEvent");

	wcscpy(UserEventName, PreShareMemory);
	wcscat(UserEventName, Prefix);
	wcscat(UserEventName, L"UserEvent");

	DbgPrint("SharedMemory : %ws\n", SharedMemoryName);
	DbgPrint("KernelEvent : %ws\n", KernelEventName);
	DbgPrint("UserEvent : %ws\n", UserEventName);

	RtlInitUnicodeString(&uName, SharedMemoryName);
	InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	ntStatus = ZwOpenSection(
		pSection,
		SECTION_ALL_ACCESS,
		&objAttributes
	);
	if (ntStatus != STATUS_SUCCESS)
	{
		DbgPrint("ZwOpenSection fail! Status: 0x%x\n", ntStatus);
		return ntStatus;
	}

	ntStatus = ZwMapViewOfSection(
		*pSection,
		NtCurrentProcess(),
		pSharedSection,
		0,
		lMaxSize.LowPart,
		NULL,
		&ulViewSize,
		ViewShare,
		0,
		PAGE_READWRITE | PAGE_NOCACHE
	);
	if (ntStatus != STATUS_SUCCESS)
	{
		DbgPrint("ZwMapViewOfSection fail! Status: 0x%x\n", ntStatus);
		return ntStatus;
	}

	RtlInitUnicodeString(&uName, KernelEventName);
	InitializeObjectAttributes(
		&objAttributes,
		&uName,
		OBJ_CASE_INSENSITIVE,
		0, 0);
	ntStatus = ZwOpenEvent(&hEvent[0], EVENT_ALL_ACCESS, &objAttributes);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
		return ntStatus;
	}
	RtlInitUnicodeString(&uName, UserEventName);
	InitializeObjectAttributes(
		&objAttributes,
		&uName,
		OBJ_CASE_INSENSITIVE,
		0, 0);
	ntStatus = ZwOpenEvent(&hEvent[1], EVENT_ALL_ACCESS, &objAttributes);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
		return ntStatus;
	}

	return ntStatus;
}

NTSTATUS
IPC_Init(
	LONG Type,
	PWCH Prefix
)
{
	PAGED_CODE();

	NTSTATUS ntStatus = STATUS_SUCCESS;
	HANDLE thread;
	pPOPThreadData pThreadData;

	switch (Type)
	{
	case 0:
		KeInitializeGuardedMutex(&ObMutex);
		g_bObCallBack = TRUE;
		break;
	case 1:
		KeInitializeGuardedMutex(&FsMutex);
		g_bFsCallBack = TRUE;
		break;
	case 2:
		KeInitializeGuardedMutex(&RegMutex);
		g_bRegCallBack = TRUE;
		break;
	default:
		DbgPrint("IPC_Init Unknown Type : %ld\n", Type);
		return STATUS_INVALID_PARAMETER;
		break;
	}

	pThreadData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(POPThreadData), 'proc');
	pThreadData->Type = Type;
	wcscpy(pThreadData->Prefix, Prefix);

	if (Type == 1)
	{
		PsCreateSystemThread(
			&thread, THREAD_ALL_ACCESS,
			NULL, NULL, NULL,
			POPDataThread, pThreadData
		);
		ZwClose(thread);
	}
	else
	{
		IoCreateSystemThread(
			g_pDriverObject,
			&thread, THREAD_ALL_ACCESS,
			NULL, NULL, NULL,
			POPDataThread, pThreadData
		);
		ZwClose(thread);
	}

	return ntStatus;
}

VOID
POPDataThread(
	PVOID ThreadContext
)
{
	//PAGED_CODE();

	NTSTATUS ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
	pPOPThreadData pThreadData = ThreadContext;
	LONG Type;
	HANDLE hSection = NULL;
	HANDLE hSharedSection = NULL;
	HANDLE hEvent[2] = { NULL, };
	PKGUARDED_MUTEX pTargetMutex = NULL;
	PLISTDATA* pTargetList = NULL;
	LARGE_INTEGER interval;
	PVOID tempData = NULL;
	PBOOLEAN pTargetExit = NULL;

	interval.QuadPart = (1 * -10 * 1000 * 1000);

	Type = pThreadData->Type;

	ntStatus = OpenSharedMemory(
		pThreadData->Prefix,
		&hSection,
		&hSharedSection,
		hEvent
	);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("OpenSharedMemory fail : 0x%x\n", ntStatus);
		ExFreePool(pThreadData);
		return;
	}

	ExFreePool(pThreadData);

	switch (Type)
	{
	case 0:
		pTargetMutex = &ObMutex;
		pTargetList = g_pObList;
		pTargetExit = &g_bObCallBack;
		break;
	case 1:
		pTargetMutex = &FsMutex;
		pTargetList = g_pFsList;
		pTargetExit = &g_bFsCallBack;
		break;
	case 2:
		pTargetMutex = &RegMutex;
		pTargetList = g_pRegList;
		pTargetExit = &g_bRegCallBack;
		break;
	default:
		DbgPrint("IPC,POPDataThread Unknown Type : %ld\n", Type);
		return;
		break;
	}

	while (*pTargetExit)
	{
		KeAcquireGuardedMutex(pTargetMutex);

		if (pTargetList[0] == NULL)
		{
			//DbgPrint("g_pRegList[0] is NULL\n");
			KeReleaseGuardedMutex(pTargetMutex);
			KeDelayExecutionThread(KernelMode, FALSE, &interval);
			continue;
		}

		if (pTargetList[0]->Data == NULL)
		{
			DbgPrint("g_pRegList[0] No Data\n");
			PVOID pCleanTarget = pTargetList[0];
			pTargetList[0] = pTargetList[0]->NextData;
			ExFreePool(pCleanTarget);
			KeReleaseGuardedMutex(pTargetMutex);
			KeDelayExecutionThread(KernelMode, FALSE, &interval);
			continue;
		}

		ZwWaitForSingleObject(hEvent[0], FALSE, NULL);

		switch (Type)
		{
		case 0:
			RtlCopyBytes(hSharedSection, pTargetList[0]->Data, sizeof(OBDATA));
		break;
		case 1:
			tempData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(FSDATA), 'ipc');
			if (tempData != NULL)
			{
				((PFSDATA)tempData)->MajorFunction = ((PFSDATA2)pTargetList[0]->Data)->MajorFunction;
				((PFSDATA)tempData)->PID = ((PFSDATA2)pTargetList[0]->Data)->PID;
				((PFSDATA)tempData)->SystemTick = ((PFSDATA2)pTargetList[0]->Data)->SystemTick;
				if (((PFSDATA2)pTargetList[0]->Data)->FileName != NULL)
				{
					wcscpy(((PFSDATA)tempData)->FileName, ((PFSDATA2)pTargetList[0]->Data)->FileName);
					ExFreePool(((PFSDATA2)pTargetList[0]->Data)->FileName);
				}
				RtlCopyBytes(hSharedSection, tempData, sizeof(FSDATA));
				ExFreePool(tempData);
			}
			else
			{
				if (((PFSDATA2)pTargetList[0]->Data)->FileName != NULL)
					ExFreePool(((PFSDATA2)pTargetList[0]->Data)->FileName);
			}
			break;
		case 2:
			tempData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(REGDATA), 'ipc');
			if (tempData != NULL)
			{
				((PREGDATA)tempData)->NotifyClass = ((PREGDATA2)pTargetList[0]->Data)->NotifyClass;
				((PREGDATA)tempData)->PID = ((PREGDATA2)pTargetList[0]->Data)->PID;
				((PREGDATA)tempData)->SystemTick = ((PREGDATA2)pTargetList[0]->Data)->SystemTick;
				if (((PREGDATA2)pTargetList[0]->Data)->RegistryFullPath != NULL)
				{
					wcscpy(((PREGDATA)tempData)->RegistryFullPath, ((PREGDATA2)pTargetList[0]->Data)->RegistryFullPath);
					ExFreePool(((PREGDATA2)pTargetList[0]->Data)->RegistryFullPath);
				}
				RtlCopyBytes(hSharedSection, tempData, sizeof(REGDATA));
				ExFreePool(tempData);
			}
			else
			{
				if (((PREGDATA2)pTargetList[0]->Data)->RegistryFullPath != NULL)
					ExFreePool(((PREGDATA2)pTargetList[0]->Data)->RegistryFullPath);
			}
		break;
		default:
			break;
		}

		ZwSetEvent(hEvent[1], NULL);

		PLISTDATA pCleanTarget = pTargetList[0];
		pTargetList[0] = pTargetList[0]->NextData;
		ExFreePool(pCleanTarget->Data);
		if (pTargetList[1] == pCleanTarget)
			pTargetList[1] = NULL;
		ExFreePool(pCleanTarget);

		KeReleaseGuardedMutex(pTargetMutex);
	}

	ZwUnmapViewOfSection(NtCurrentProcess(), hSharedSection);
	ZwClose(hSection);

	KeAcquireGuardedMutex(pTargetMutex);

	while (pTargetList[0] != NULL)
	{
		PLISTDATA pCleanTarget = pTargetList[0];
		pTargetList[0] = pTargetList[0]->NextData;
		if (pCleanTarget->Data != NULL)
		{
			switch (Type)
			{
			case 1:
				if (((PFSDATA2)pCleanTarget->Data)->FileName != NULL) ExFreePool(((PFSDATA2)pCleanTarget->Data)->FileName);
				break;
			case 2:
				if (((PREGDATA2)pCleanTarget->Data)->RegistryFullPath != NULL) ExFreePool(((PREGDATA2)pCleanTarget->Data)->RegistryFullPath);
			default:
				break;
			}
			ExFreePool(pCleanTarget->Data);
		}
	}

	KeReleaseGuardedMutex(pTargetMutex);

	DbgPrint("%d POPThread Exit\n", Type);
}

VOID
DataInsertThread(
	PVOID ThreadContext
)
{
	NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
	PCOMDATA pComData = ThreadContext;
	LONG Type = 99;
	PVOID pData = NULL;
	PLISTDATA* pTargetList = NULL;
	PKGUARDED_MUTEX pTargetMutex = NULL;
	PBOOLEAN pTargetExit = NULL;

	pData = pComData->Data;
	Type = pComData->Type;
	ExFreePool(pComData);

	switch (Type)
	{
	case 0:
		pTargetList = g_pObList;
		pTargetMutex = &ObMutex;
		pTargetExit = &g_bObCallBack;
		break;
	case 1:
		pTargetList = g_pFsList;
		pTargetMutex = &FsMutex;
		pTargetExit = &g_bFsCallBack;
		break;
	case 2:
		pTargetList = g_pRegList;
		pTargetMutex = &RegMutex;
		pTargetExit = &g_bRegCallBack;
		break;
	default:
		ntStatus = STATUS_INVALID_PARAMETER;
		goto Exit;
		break;
	}

	if (!*pTargetExit)
	{
		pTargetMutex = NULL;
		ntStatus = STATUS_THREAD_IS_TERMINATING;
		goto Exit;
	}

	KeAcquireGuardedMutex(pTargetMutex);

	if (!*pTargetExit)
	{
		ntStatus = STATUS_THREAD_IS_TERMINATING;
		goto Exit;
	}

	if (pTargetList[1] == NULL)
	{
		pTargetList[1] = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(LISTDATA), 'proc');
		if (pTargetList[1] == NULL)
			goto Exit;
	}
	else
	{
		pTargetList[1]->NextData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(LISTDATA), 'proc');
		if (pTargetList[1]->NextData == NULL)
			goto Exit;

		pTargetList[1] = pTargetList[1]->NextData;
	}
	pTargetList[1]->Data = pData;
	
	if (pTargetList[0] == NULL)
		pTargetList[0] = pTargetList[1];

	ntStatus = STATUS_SUCCESS;

Exit:
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IPC, DataInsertThread fail : 0x%x\n", ntStatus);

		switch (Type)
		{
		case 1:
			if (pData != NULL)
				if (((PFSDATA2)pData)->FileName != NULL)
					ExFreePool(((PFSDATA2)pData)->FileName);
			break;
		case 2:
			if (pData != NULL)
				if (((PREGDATA2)pData)->RegistryFullPath != NULL)
					ExFreePool(((PREGDATA2)pData)->RegistryFullPath);
		default:
			break;
		}
		if (pData != NULL)
			ExFreePool(pData);
	}

	if(pTargetMutex != NULL) KeReleaseGuardedMutex(pTargetMutex);
}

/// <summary>
/// Callback에서 데이터 저장을 위해 부르는 함수
/// </summary>
/// <param name="pData">
/// OB-OBDATA FS-FSDATA REG-FSDATA
/// </param>
/// <param name="Type">
/// OB-0 FS-1 REG-2
/// </param>
VOID
CreateData(
	PVOID pData,
	LONG Type
)
{
	PAGED_CODE();

	HANDLE thread = NULL;
	PCOMDATA ComData = NULL;

	if (pData == NULL)
		return;

	ComData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(COMDATA), 'proc');

	if (ComData == NULL)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
			"IOCTL,CreateData,ExAllocatePool2 Fail\n");

		// REGDATA2, FSDATA2는 포인터를 가지고 있으므로 따로 메모리 해제해줘야 함
		switch (Type)
		{
		case 1:
			if (((PFSDATA2)pData)->FileName != NULL)
				ExFreePool(((PFSDATA2)pData)->FileName);
			break;
		case 2:
			if (((PREGDATA2)pData)->RegistryFullPath != NULL)
				ExFreePool(((PREGDATA2)pData)->RegistryFullPath);
			break;
		default:
			break;
		}
		ExFreePool(pData);
		return;
	}

	ComData->Data = pData;
	ComData->Type = Type;

	if (Type == 1)
	{
		PsCreateSystemThread(
			&thread, THREAD_ALL_ACCESS,
			NULL, NULL, NULL,
			DataInsertThread, ComData
		);
		ZwClose(thread);
	}
	else
	{
		IoCreateSystemThread(
			g_pDriverObject,
			&thread, THREAD_ALL_ACCESS,
			NULL, NULL, NULL,
			DataInsertThread, ComData
		);
		ZwClose(thread);
	}
}

