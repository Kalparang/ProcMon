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
// Driver and device names
// It is important to change the names of the binaries
// in the sample code to be unique for your own use.
//

#define TD_DRIVER_NAME             L"ObCallbackTest"
#define TD_DRIVER_NAME_WITH_EXT    L"ObCallbackTest.sys"

#define TD_NT_DEVICE_NAME          L"\\Device\\ObCallbackTest"
#define TD_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\ObCallbackTest"
#define TD_WIN32_DEVICE_NAME       L"\\\\.\\ObCallbackTest"

// RegFilter

#define NT_DEVICE_NAME          L"\\Device\\RegFltr"
#define DOS_DEVICES_LINK_NAME   L"\\DosDevices\\RegFltr"
#define WIN32_DEVICE_NAME       L"\\\\.\\RegFltr"

//
// SDDL string used when creating the device. This string
// limits access to this driver to system and admins only.
//

#define DEVICE_SDDL             L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"

#define MAX_ALTITUDE_BUFFER_LENGTH 10

//

#define NAME_SIZE   200

#define TD_INVALID_CALLBACK_ID ((ULONG)-1)
