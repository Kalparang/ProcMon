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

extern PDRIVER_OBJECT g_pDriverObject;
extern BOOLEAN g_bExit;

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

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE thread;
	pPOPThreadData pThreadData;

	switch (Type)
	{
	case 0:
		KeInitializeGuardedMutex(&ObMutex);
		break;
	case 1:
		KeInitializeGuardedMutex(&FsMutex);
		break;
	case 2:
		KeInitializeGuardedMutex(&RegMutex);
		break;
	default:
		DbgPrint("IPC_Init Unknown Type : %ld\n", Type);
		return STATUS_INVALID_PARAMETER;
		break;
	}

	pThreadData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(POPThreadData), 'proc');
	pThreadData->Type = Type;
	wcscpy(pThreadData->Prefix, Prefix);

	IoCreateSystemThread(
		g_pDriverObject,
		&thread, THREAD_ALL_ACCESS,
		NULL, NULL, NULL,
		POPDataThread, pThreadData
	);
	ZwClose(thread);

	return ntStatus;
}

VOID
POPDataThread(
	PVOID ThreadContext
)
{
	PAGED_CODE();

	NTSTATUS ntStatus;
	pPOPThreadData pThreadData = ThreadContext;
	LONG Type;
	HANDLE hSection = NULL;
	HANDLE hSharedSection = NULL;
	HANDLE hEvent[2] = { NULL, };

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
		break;
	case 1:
		break;
	case 2:
	{
		PREGDATA pRegData = hSharedSection;

		while (1)
		{
			KeAcquireGuardedMutex(&RegMutex);
			DbgPrint("2\n");
			ZwWaitForSingleObject(hEvent[0], FALSE, NULL);
			DbgPrint("3\n");

			DbgPrint("PID : %ld\n", pRegData->PID++);
			DbgPrint("Noti : %ld\n", pRegData->NotifyClass++);
			DbgPrint("Tick : %lld\n", pRegData->SystemTick++);
			DbgPrint("Tick : %ws\n", pRegData->RegistryFullPath);

			ZwSetEvent(hEvent[1], NULL);
			KeReleaseGuardedMutex(&RegMutex);
		}
	}
	break;
	default:
		break;
	}

	ZwUnmapViewOfSection(NtCurrentProcess(), hSharedSection);
	ZwClose(hSection);
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

	//HANDLE thread = NULL;
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

	//IoCreateSystemThread(
	//	g_pDriverObject,
	//	&thread, THREAD_ALL_ACCESS,
	//	NULL, NULL, NULL,
	//	POPDataThread, ComData
	//);
	//ZwClose(thread);
}
