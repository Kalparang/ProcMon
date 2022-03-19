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

/// @brief	현재 프로세스가 "System" 이라는 가정하에 EPROCESS::ImageFileName 필드의 offset 을 찾는다. 
///			반드시 DriverEntry 함수에서 호출되어야 함 (System process context 에서 실행)
BOOLEAN SetProcessNameOffset(
);

/// @brief	EPROCESS 에서 image name 을 찾아 리턴한다.
const char* get_process_name(
	_In_ PEPROCESS eprocess,
	_Out_ char* NameCopy
);