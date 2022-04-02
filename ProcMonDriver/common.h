#pragma once
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
// Device type           -- in the "User Defined" range."
//
#define SIOCTL_TYPE 40000
//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//
#define IOCTL_CALLBACK_START \
    CTL_CODE( SIOCTL_TYPE, 0x900, METHOD_IN_DIRECT, FILE_ANY_ACCESS  )

#define IOCTL_CALLBACK_STOP \
    CTL_CODE( SIOCTL_TYPE, 0x901, METHOD_IN_DIRECT , FILE_ANY_ACCESS  )

#define IOCTL_TEST \
    CTL_CODE( SIOCTL_TYPE, 0x902, METHOD_IN_DIRECT , FILE_ANY_ACCESS  )

//
// Driver and device names
// It is important to change the names of the binaries
// in the sample code to be unique for your own use.
//

#define NT_DEVICE_NAME L"\\Device\\ProcMonDriver"
#define DOS_DEVICE_NAME L"\\DosDevices\\ProcMonDevice"

#define OB_DRIVER_NAME             L"ProcMonOB"
#define OB_DRIVER_NAME_WITH_EXT    L"ProcMonOB.sys"

#define OB_DEVICE_NAME          L"\\Device\\ProcMonOB"
#define OB_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\ProcMonobc"

#define FS_DEVICE_NAME          L"\\Device\\ProcMonFS"
#define FS_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\ProcMonfsy"

// RegFilter

#define REG_DEVICE_NAME          L"\\Device\\ProcMonRF"
#define REG_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\ProcMonreg"

//
// SDDL string used when creating the device. This string
// limits access to this driver to system and admins only.
//

#define DEVICE_SDDL             L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"

#define MAX_ALTITUDE_BUFFER_LENGTH 10

//

#define NAME_SIZE   200

#define TD_INVALID_CALLBACK_ID ((ULONG)-1)

/// <summary>
/// IOCTL에서 IOCTL_CALLBACK_START, IOCTL_CALLBACK_STOP에서 쓰이는 inputbuffer
/// </summary>
typedef struct _ioCallbackControl
{
    LONG Type;
    WCHAR CallbackPrefix[32];
} ioCallbackControl, * PioCallbackControl;