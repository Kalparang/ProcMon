#include "common.h"

#define NT_PROCNAMELEN	 16

static ULONG _process_name_offset = 0x00000000;

ULONG g_DebugLevel = /*DPFLTR_TRACE_LEVEL*/DPFLTR_INFO_LEVEL;

// Buffers for debug messages are allocated globally instead of 
// on a stack, therefore we need g_DebugBufferBusy flags to 
// protect access to them.
//
ULONG g_DebugBufferBusy[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
CHAR  g_DebugBuffer[NUMBER_DEBUG_BUFFERS][1024];

VOID drv_debug_print(IN UINT32 level, IN const char* function, IN const char* format, IN ...)
{
	ULONG i;
	va_list vaList;
	va_start(vaList, format);

	CHAR ProcName[NT_PROCNAMELEN] = { 0 };

	// check mask for debug area and debug level
	//
	if (level <= g_DebugLevel)
	{
		// find a free buffer
		//
		for (i = 0; i < NUMBER_DEBUG_BUFFERS; ++i)
		{
			if (InterlockedCompareExchange((LONG*)&g_DebugBufferBusy[i], 1, 0) == 0)
			{
				__try
				{
					if (TRUE != NT_SUCCESS(RtlStringCbVPrintfA(
						g_DebugBuffer[i],
						sizeof(g_DebugBuffer[i]),
						format,
						vaList
					)))
					{
						return;
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					return;
				}

				// get process name 
				get_process_name(PsGetCurrentProcess(), ProcName);

				if (DPFLTR_ERROR_LEVEL == level)
				{
					DbgPrintEx(
						DPFLTR_IHVDRIVER_ID,
						DPFLTR_ERROR_LEVEL,
						"(IRQL %2.2d): %-16s(%04u:%04u): [ERR ] %s(), %s\n",
						KeGetCurrentIrql(),
						ProcName, PsGetCurrentProcessId(), PsGetCurrentThreadId(),
						function,
						g_DebugBuffer[i]
					);
				}
				else if (DPFLTR_WARNING_LEVEL == level)
				{
					DbgPrintEx(
						DPFLTR_IHVDRIVER_ID,
						DPFLTR_WARNING_LEVEL | DPFLTR_MASK,
						"(IRQL %2.2d): %-16s(%04u:%04u): [WARN] %s(), %s\n",
						KeGetCurrentIrql(),
						ProcName, PsGetCurrentProcessId(), PsGetCurrentThreadId(),
						function,
						g_DebugBuffer[i]
					);
				}
				else if (DPFLTR_TRACE_LEVEL == level)
				{
					DbgPrintEx(
						DPFLTR_IHVDRIVER_ID,
						DPFLTR_TRACE_LEVEL | DPFLTR_MASK,
						"(IRQL %2.2d): %-16s(%04u:%04u): [TRCE] %s(), %s\n",
						KeGetCurrentIrql(),
						ProcName, PsGetCurrentProcessId(), PsGetCurrentThreadId(),
						function,
						g_DebugBuffer[i]
					);
				}
				else
				{
					DbgPrintEx(
						DPFLTR_IHVDRIVER_ID,
						DPFLTR_INFO_LEVEL | DPFLTR_MASK,
						"(IRQL %2.2d): %-16s(%04u:%04u): [INFO] %s(), %s\n",
						KeGetCurrentIrql(),
						ProcName, PsGetCurrentProcessId(), PsGetCurrentThreadId(),
						function,
						g_DebugBuffer[i]
					);
				}

				InterlockedExchange((LONG*)&g_DebugBufferBusy[i], 0);
				break;
			}
		}
	}

	va_end(vaList);
}

/// @brief	���� ���μ����� "System" �̶�� �����Ͽ� EPROCESS::ImageFileName �ʵ��� offset �� ã�´�. 
///			�ݵ�� DriverEntry �Լ����� ȣ��Ǿ�� �� (System process context ���� ����)
BOOLEAN SetProcessNameOffset(
)
{
	//
	// ���� process�� SYSTEM process�� �����Ͽ� SYSTEM�̶� ���ڿ��� ã�´�.
	//

	PEPROCESS curproc = PsGetCurrentProcess();
	for (int i = 0; i < 3 * PAGE_SIZE; i++)
	{
		if (!_strnicmp("system", (PCHAR)curproc + i, strlen("system")))
		{
			_process_name_offset = i;
			return TRUE;
		}
	}

	//
	// Name not found - oh, well
	//
	return FALSE;
}

/// @brief	EPROCESS ���� image name �� ã�� �����Ѵ�.
const char* get_process_name(
	_In_ PEPROCESS eprocess,
	_Out_ char* NameCopy
)
{
	PEPROCESS curproc = NULL;
	char* nameptr = NULL;

	//
	// We only try and get the name if we located the name offset
	//
	if (0 != _process_name_offset)
	{
		//
		// Get a pointer to the current process block
		//
		curproc = eprocess;

		//
		// Dig into it to extract the name. Make sure to leave enough room
		// in the buffer for the appended process ID.
		//
		nameptr = (PCHAR)curproc + _process_name_offset;
		strncpy(NameCopy, nameptr, NT_PROCNAMELEN - 1);
		NameCopy[NT_PROCNAMELEN - 1] = 0;
		///!    sprintf( name + strlen(name), ":%d", (ULONG) PsGetCurrentProcessId());
	}
	else
	{
		strncpy(NameCopy, "???", NT_PROCNAMELEN - 1);
	}

	return NameCopy;
}