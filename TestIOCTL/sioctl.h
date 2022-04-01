#pragma once

#include <windows.h>
#include <winioctl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strsafe.h>

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

#define DRIVER_NAME       L"ProcMonDriver"

BOOLEAN
InstallDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName,
    _In_ LPCTSTR    ServiceExe
);


BOOLEAN
RemoveDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName
);

BOOLEAN
StartDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName
);

BOOLEAN
StopDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName
);

BOOLEAN
ManageDriver(
    _In_ LPCTSTR  DriverName,
    _In_ LPCTSTR  ServiceName,
    _In_ USHORT   Function
);

BOOLEAN
SetupDriverName(
    _Inout_updates_bytes_all_(BufferLength) PWCHAR DriverLocation,
    _In_ ULONG BufferLength
);