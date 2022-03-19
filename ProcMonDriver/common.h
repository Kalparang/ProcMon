#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>

#define NUMBER_DEBUG_BUFFERS    (sizeof(g_DebugBufferBusy)/sizeof(g_DebugBufferBusy[0]))

/**	-----------------------------------------------------------------------
	\brief	debug message output routine

	\param
		IN  level
				Debug level (DBG_ERR, DBG_INFO, etc..)

		IN  format
				Debug Message format

	\return
		NONE
	\code
	\endcode
-------------------------------------------------------------------------*/
VOID drv_debug_print(IN UINT32 Level, IN const char* Function, IN const char* Format, IN ...);

//__inline void drv_debug_print(IN UINT32 level, IN const char* function, IN const char* format, IN ...)
//{
//    UNREFERENCED_PARAMETER(level);
//    UNREFERENCED_PARAMETER(function);
//    UNREFERENCED_PARAMETER(format);
//}

/// @brief	���� ���μ����� "System" �̶�� �����Ͽ� EPROCESS::ImageFileName �ʵ��� offset �� ã�´�. 
///			�ݵ�� DriverEntry �Լ����� ȣ��Ǿ�� �� (System process context ���� ����)
BOOLEAN SetProcessNameOffset(
);

/// @brief	EPROCESS ���� image name �� ã�� �����Ѵ�.
const char* get_process_name(
	_In_ PEPROCESS eprocess,
	_Out_ char* NameCopy
);