#include "pch.h"
#include "IOCTL.h"

extern PDRIVER_OBJECT g_pDriverObject;

KGUARDED_MUTEX ObMutex;
KGUARDED_MUTEX FsMutex;
KGUARDED_MUTEX RegMutex;

// DataLinkedLisk Queue
// 0 - Head
// 1 - Tail
PDATALIST ObList[2] = { NULL, };
PDATALIST FsList[2] = { NULL, };
PDATALIST RegList[2] = { NULL, };

//POPData에서 사용할 데이터
//List[0]을 POP
POBDATA pObOutDataSave = NULL;
PFSDATA pFsOutDataSave = NULL;
PREGDATA pRegOutDataSave = NULL;

//
size_t FsOutPos;
size_t RegOutPos;

VOID
IOUnload()
{

}

VOID
POPData(
	ULONG Type,
	PVOID OutData
)
{
	if (OutData == NULL)
		return;

	switch (Type)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
	{
		//if (sizeof(*(PREGOUTDATA)OutData) != sizeof(REGOUTDATA))
		//	return;

		if (pRegOutDataSave == NULL)
		{
			while (RegList[0] == NULL)
			{
				//Flag 구현
				LARGE_INTEGER interval;
				interval.QuadPart = (1 * -10 * 1000 * 1000);

				KeDelayExecutionThread(KernelMode, FALSE, &interval);
			}

			PDATALIST pDataClean = RegList[0];
			pRegOutDataSave = pDataClean->Data;
			RegList[0] = (PDATALIST)pDataClean->NextData;
			ExFreePool(pDataClean);
		}

		PREGOUTDATA pRegOutData = OutData;

		pRegOutData->RegData.NotifyClass = pRegOutDataSave->NotifyClass;
		pRegOutData->RegData.PID = pRegOutDataSave->PID;
		pRegOutData->RegData.RegistryFullPath = NULL;
		pRegOutData->RegData.SystemTick = pRegOutDataSave->SystemTick;

		pRegOutData->RegistryPathLength = wcslen(pRegOutDataSave->RegistryFullPath);
		size_t CopyLength = pRegOutData->RegistryPathLength - RegOutPos;

		if (REGSTRINGSIZE <= CopyLength)
			CopyLength = REGSTRINGSIZE - 1;

		wcsncpy(pRegOutData->RegistryPath,
			pRegOutDataSave->RegistryFullPath + RegOutPos,
			CopyLength);
		pRegOutData->RegistryPath[CopyLength + 1] = L'\0';
		RegOutPos += CopyLength;

		if (RegOutPos == wcslen(pRegOutDataSave->RegistryFullPath))
		{
			RegOutPos = 0;
			ExFreePool(pRegOutDataSave->RegistryFullPath);
			ExFreePool(pRegOutDataSave);
			pRegOutDataSave = NULL;
		}

		OutData = pRegOutData;
	}
		break;
	default:
		break;
	}
}

VOID
TestPOPThread(
	_In_ PVOID ThreadContext
)
{
	UNREFERENCED_PARAMETER(ThreadContext);

	LARGE_INTEGER interval;
	interval.QuadPart = (3 * -10 * 1000 * 1000);

	while (1)
	{
		REGOUTDATA OutData;
		POPData(2, &OutData);
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
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
					
					return;
				}
			}
			while (RegTarget->NextData != NULL) RegTarget = (PDATALIST)RegTarget->NextData;

			if (RegTarget == NULL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
					"IOCTL,DataInsertThread,ExAllocatePool2 fail\n");
				ExFreePool(ComData->Data);
				ExFreePool(ComData);
				return;
			}

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
	HANDLE thread = NULL;
	PCOMDATA ComData = NULL;

	if (pData == NULL)
		return;

	ComData = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(COMDATA), 'proc');

	if (ComData == NULL)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
			"IOCTL,CreateData,ExAllocatePool2 Fail\n");
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

VOID
IOInit()
{
	FsOutPos = 0;
	RegOutPos = 0;
	KeInitializeGuardedMutex(&ObMutex);
	KeInitializeGuardedMutex(&FsMutex);
	KeInitializeGuardedMutex(&RegMutex);

	//HANDLE thread;
	//IoCreateSystemThread(
	//	g_pDriverObject,
	//	&thread, THREAD_ALL_ACCESS,
	//	NULL, NULL, NULL,
	//	TestPOPThread, NULL
	//);
	//ZwClose(thread);
}

NTSTATUS
ioctlDeviceControl(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
);