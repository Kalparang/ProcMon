#pragma once

//#include <ntifs.h>
//#include <fltKernel.h>
//#include <ntstrsafe.h>
//#include <ntintsafe.h>
#include <ntddk.h>

#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

//
// TD_ASSERT
//
// This macro is identical to NT_ASSERT but works in fre builds as well.
//
// It is used for error checking in the driver in cases where
// we can't easily report the error to the user mode app, or the
// error is so severe that we should break in immediately to
// investigate.
//
// It's better than DbgBreakPoint because it provides additional info
// that can be dumped with .exr -1, and individual asserts can be disabled
// from kd using 'ahi' command.
//

#define TD_ASSERT(_exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", L#_exp), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)

//
// Driver and device names
// It is important to change the names of the binaries
// in the sample code to be unique for your own use.
//

#define TD_DRIVER_NAME             L"ObCallbackTest"
#define TD_DRIVER_NAME_WITH_EXT    L"ObCallbackTest.sys"

#define TD_NT_DEVICE_NAME          L"\\Device\\ObCallbackTest"
#define TD_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\ObCallbackTest"
#define TD_WIN32_DEVICE_NAME       L"\\\\.\\ObCallbackTest"


#define NAME_SIZE   200

#define TD_INVALID_CALLBACK_ID ((ULONG)-1)

//
// IOCTLs exposed by the driver.
//

// #define TD_IOCTL_REGISTER_CALLBACK   CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 0), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
// #define TD_IOCTL_UNREGISTER_CALLBACK CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 1), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define TD_IOCTL_PROTECT_NAME_CALLBACK        CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 2), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define TD_IOCTL_UNPROTECT_CALLBACK           CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 3), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)


#define TDProtectName_Protect  0            // name of programs to proect and filter out the desiredAccess on Process Open
#define TDProtectName_Reject   1            // name of programs to reject during ProcessCreate

#define NUMBER_DEBUG_BUFFERS    (sizeof(g_DebugBufferBusy)/sizeof(g_DebugBufferBusy[0]))

//
// Structures used by TD_IOCTL_PROTECTNAME
//

typedef struct _TD_PROTECTNAME_INPUT {
	ULONG Operation;
	WCHAR Name[NAME_SIZE + 1];      // what is the filename to protect - extra wchar for forced NULL
}
TD_PROTECTNAME_INPUT, * PTD_PROTECTNAME_INPUT;

//
// Structures used by TD_IOCTL_UNPROTECT_CALLBACK
//

typedef struct _TD_UNPROTECT_CALLBACK_INPUT {
	ULONG UnusedParameter;
}
TD_UNPROTECT_CALLBACK_INPUT, * PTD_UNPROTECT_CALLBACK_INPUT;

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