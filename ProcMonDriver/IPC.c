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

PVOID	g_pSharedSection = NULL;
HANDLE g_hSection = NULL;
PIPCSTRUCT g_pIPCStruct = NULL;

HANDLE g_hObEvent[2];
HANDLE g_hFsEvent[2];
HANDLE g_hRegEvent[2];

KGUARDED_MUTEX ObMutex;
KGUARDED_MUTEX FsMutex;
KGUARDED_MUTEX RegMutex;

// DataLinkedLisk Queue
// 0 - Head
// 1 - Tail
PDATALIST ObList[2] = { NULL, };
PDATALIST FsList[2] = { NULL, };
PDATALIST RegList[2] = { NULL, };

extern PDRIVER_OBJECT g_pDriverObject;
extern BOOLEAN g_bExit;

VOID
POPDataThread(
	PVOID ThreadContext
);

NTSTATUS CreateSharedMemory(
)
{
	PAGED_CODE();

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING uName = { 0 };
	OBJECT_ATTRIBUTES objAttributes = { 0 };
	SIZE_T ulViewSize = 0;
	HANDLE thread;
	PLONG ThreadContext;

	KeInitializeGuardedMutex(&ObMutex);
	KeInitializeGuardedMutex(&FsMutex);
	KeInitializeGuardedMutex(&RegMutex);

	RtlInitUnicodeString(&uName, PreShareMemory SharedSectionName);
	InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	LARGE_INTEGER lMaxSize = { 0 };
	lMaxSize.HighPart = 0;
	lMaxSize.LowPart = IPC_BUFFER2;
	ntStatus = ZwOpenSection(
		&g_hSection,
		SECTION_ALL_ACCESS,
		&objAttributes
	);
	if (ntStatus != STATUS_SUCCESS)
	{
		DbgPrint("ZwOpenSection fail! Status: 0x%x\n", ntStatus);
		ClearSharedMemory();
		return ntStatus;
	}

	ntStatus = ZwMapViewOfSection(
		g_hSection,
		NtCurrentProcess(),
		&g_pSharedSection,
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
		ClearSharedMemory();
		return ntStatus;
	}

	g_pIPCStruct = g_pSharedSection;

	//RtlInitUnicodeString(&uName, PreShareMemory ObKernelEventName);
	//InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	//ntStatus = ZwOpenEvent(&g_hObEvent[0], EVENT_ALL_ACCESS, &objAttributes);
	//if (!NT_SUCCESS(ntStatus))
	//{
	//	DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
	//	ClearSharedMemory();
	//	return ntStatus;
	//}
	//RtlInitUnicodeString(&uName, PreShareMemory ObUserEventName);
	//InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	//ntStatus = ZwOpenEvent(&g_hObEvent[1], EVENT_ALL_ACCESS, &objAttributes);
	//if (!NT_SUCCESS(ntStatus))
	//{
	//	DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
	//	ClearSharedMemory();
	//	return ntStatus;
	//}

	//RtlInitUnicodeString(&uName, PreShareMemory FsKernelEventName);
	//InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	//ntStatus = ZwOpenEvent(&g_hFsEvent[0], EVENT_ALL_ACCESS, &objAttributes);
	//if (!NT_SUCCESS(ntStatus))
	//{
	//	DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
	//	ClearSharedMemory();
	//	return ntStatus;
	//}
	//RtlInitUnicodeString(&uName, PreShareMemory FsUserEventName);
	//InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	//ntStatus = ZwOpenEvent(&g_hFsEvent[1], EVENT_ALL_ACCESS, &objAttributes);
	//if (!NT_SUCCESS(ntStatus))
	//{
	//	DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
	//	ClearSharedMemory();
	//	return ntStatus;
	//}

	RtlInitUnicodeString(&uName, PreShareMemory RegKernelEventName);
	InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	ntStatus = ZwOpenEvent(&g_hRegEvent[0], EVENT_ALL_ACCESS, &objAttributes);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
		ClearSharedMemory();
		return ntStatus;
	}
	RtlInitUnicodeString(&uName, PreShareMemory RegUserEventName);
	InitializeObjectAttributes(&objAttributes, &uName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	ntStatus = ZwOpenEvent(&g_hRegEvent[1], EVENT_ALL_ACCESS, &objAttributes);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
		ClearSharedMemory();
		return ntStatus;
	}

	ThreadContext = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(LONG), 'proc');
	*ThreadContext = 2;
	IoCreateSystemThread(
		g_pDriverObject,
		&thread, THREAD_ALL_ACCESS,
		NULL, NULL, NULL,
		POPDataThread, ThreadContext
	);
	ZwClose(thread);

	return ntStatus;
}

VOID ClearSharedMemory(
)
{
	if (g_hObEvent[0] != NULL)
		ZwClose(g_hObEvent[0]);
	if (g_hObEvent[1] != NULL)
		ZwClose(g_hObEvent[1]);
	if (g_hFsEvent[0] != NULL)
		ZwClose(g_hFsEvent[0]);
	if (g_hFsEvent[1] != NULL)
		ZwClose(g_hFsEvent[1]);
	if (g_hRegEvent[0] != NULL)
		ZwClose(g_hRegEvent[0]);
	if (g_hRegEvent[1] != NULL)
		ZwClose(g_hRegEvent[1]);
	if (g_pSharedSection)
		ZwUnmapViewOfSection(NtCurrentProcess(), g_pSharedSection);
	if (g_hSection)
		ZwClose(g_hSection);
}

VOID MapViewTest(
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	SIZE_T ulViewSize = 0;
	LARGE_INTEGER lMaxSize = { 0 };
	lMaxSize.HighPart = 0;
	lMaxSize.LowPart = IPC_BUFFER2;

	if (g_hSection == NULL)
		return;

	ntStatus = ZwMapViewOfSection(
		g_hSection,
		NtCurrentProcess(),
		&g_pSharedSection,
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
		ZwClose(g_hSection);
		return;
	}
	
	ptest t1;
	HANDLE kernelSignal;
	HANDLE userSignal;

	t1 = g_pSharedSection;

	UNICODE_STRING uSectionName = { 0 };
	RtlInitUnicodeString(&uSectionName, PreShareMemory RegUserEventName);

	OBJECT_ATTRIBUTES objAttributes = { 0 };
	InitializeObjectAttributes(&objAttributes, &uSectionName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	ntStatus = ZwOpenEvent(
		&userSignal,
		EVENT_ALL_ACCESS,
		&objAttributes
	);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
		return;
	}

	RtlInitUnicodeString(&uSectionName, PreShareMemory RegKernelEventName);
	InitializeObjectAttributes(&objAttributes, &uSectionName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	ntStatus = ZwOpenEvent(
		&kernelSignal,
		EVENT_ALL_ACCESS,
		&objAttributes
	);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwOpenEvent fail : 0x%x\n", ntStatus);
		return;
	}
	
	while (1)
	{
		ZwWaitForSingleObject(kernelSignal, FALSE, NULL);

		DbgPrint("%ld\n", t1->PID++);

		ZwSetEvent(userSignal, NULL);
	}
}


VOID
POPDataThread(
	PVOID ThreadContext
)
{
	PAGED_CODE();

	LONG Type = *(LONG*)ThreadContext;
	ExFreePool(ThreadContext);

	switch (Type)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
	{
		DbgPrint("POPDataThread Registry Start\n");

		while (1)
		{
			ZwWaitForSingleObject(g_hRegEvent[0], FALSE, NULL);
			
			while (RegList[0] == NULL && !g_bExit)
			{
				//Flag 구현
				LARGE_INTEGER interval;
				interval.QuadPart = (1 * -10 * 1000 * 1000);

				DbgPrint("No RegData...\n");

				KeDelayExecutionThread(KernelMode, FALSE, &interval);
			}

			if (g_bExit)
			{
				DbgPrint("POPDataThread Registry receive ExitSignal\n");
				break;
			}

			PDATALIST pCleanData = RegList[0];
			PREGDATA pTargetData = RegList[0]->Data;
			PREGDATA pSMData = &g_pIPCStruct->RegData;

			pSMData->SystemTick = pTargetData->SystemTick;
			pSMData->PID = pTargetData->PID;
			pSMData->NotifyClass = pTargetData->NotifyClass;
			wcscpy_s(pSMData->RegistryFullPath, PROC_MAX_PATH, pTargetData->RegistryFullPath);

			RegList[0] = (PDATALIST)RegList[0]->NextData;

			ExFreePool(pTargetData);
			ExFreePool(pCleanData);

			ZwSetEvent(g_hRegEvent[1], NULL);
		}

		while (RegList[0] != NULL)
		{
			PDATALIST pCleanData = RegList[0];
			RegList[0] = (PDATALIST)RegList[0]->NextData;
			ExFreePool(pCleanData->Data);
			ExFreePool(pCleanData);
		}

		DbgPrint("POPDataThread Registry Exit\n");
	}
	break;
	default:
		break;
	}
}


/// <summary>
/// CreateData에서 호출하는 Thread
/// </summary>
/// <param name="pComData"></param>
VOID
DataInsertThread(
	_In_ PVOID pComData
)
{
	PAGED_CODE();

	PCOMDATA ComData = pComData;

	switch (ComData->Type)
	{
	case 0:
		KeAcquireGuardedMutex(&ObMutex);
		{
			PDATALIST ObTarget = ObList[1];
			while (ObTarget->NextData != NULL) ObTarget = (PDATALIST)ObTarget->NextData;

			ObTarget = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(DATALIST), 'proc');
			if (ObTarget == NULL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
					"IOCTL,DataInsertThread,ExAllocatePool2 fail\n");
				ExFreePool(ComData->Data);
				return;
			}

			ObTarget->Data = ComData->Data;
			ObList[1] = ObTarget;

			if (ObList[0] == NULL)
				ObList[0] = ObTarget;
			KeReleaseGuardedMutex(&ObMutex);
		}
		break;
	case 1:
		KeAcquireGuardedMutex(&FsMutex);
		{
			PDATALIST FsTarget = FsList[1];
			while (FsTarget->NextData != NULL) FsTarget = (PDATALIST)FsTarget->NextData;

			FsTarget = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(DATALIST), 'proc');
			if (FsTarget == NULL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
					"IOCTL,DataInsertThread,ExAllocatePool2 fail\n");
				ExFreePool(ComData->Data);
				return;
			}

			FsTarget->Data = ComData->Data;
			FsList[1] = FsTarget;

			if (FsList[0] == NULL)
				FsList[0] = FsTarget;
		}
		KeReleaseGuardedMutex(&FsMutex);
		break;
	case 2:
		KeAcquireGuardedMutex(&RegMutex);
		{
			PDATALIST RegTarget = RegList[1];
			if (RegTarget == NULL)
			{
				RegTarget = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(DATALIST), 'proc');
				if (RegTarget == NULL)
				{
					DbgPrint("IPC,DataInsertThread ExAllocatePool2 Registry Fail\n");
					ExFreePool(ComData->Data);
					ExFreePool(ComData);
					return;
				}
			}
			//while (RegTarget->NextData != NULL) RegTarget = (PDATALIST)RegTarget->NextData;

			RegTarget->Data = ComData->Data;
			RegTarget->NextData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(DATALIST), 'proc');
			RegList[1] = (PDATALIST)RegTarget->NextData;

			if (RegList[0] == NULL)
				RegList[0] = RegTarget;
		}
		KeReleaseGuardedMutex(&RegMutex);
		break;
	default:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
			"IOCTL,DataInsertThread,Unknown Type\n");
		ExFreePool(ComData->Data);
		break;
	}

	ExFreePool(ComData);
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
		ExFreePool(pData);
		return;
	}

	ComData->Data = pData;
	ComData->Type = Type;

	IoCreateSystemThread(
		g_pDriverObject,
		&thread, THREAD_ALL_ACCESS,
		NULL, NULL, NULL,
		DataInsertThread, ComData
	);
	ZwClose(thread);
}