#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <thread>

#define PROC_MAX_PATH 32767

#define PreShareMemory L"Global\\"
#define SharedSectionName L"SharedMemory"
#define ObKernelEventName L"KernelEvent"
#define ObUserEventName L"UserEvent"
#define FsKernelEventName L"KernelEvent"
#define FsUserEventName L"UserEvent"
#define RegKernelEventName L"KernelEvent"
#define RegUserEventName L"UserEvent"

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

typedef struct _ioCallbackControl
{
    LONG Type;
    WCHAR CallbackPrefix[32];
} ioCallbackControl, * PioCallbackControl;

typedef struct _OBDATA
{
    INT64 SystemTick;	// count of 100-nanosecond intervals since January 1, 1601
    HANDLE PID;
    HANDLE TargetPID;
    ULONG Operation;
    ACCESS_MASK DesiredAccess;
} OBDATA, * POBDATA;

typedef struct _FSDATA
{
    INT64 SystemTick;
    HANDLE PID;
    UCHAR MajorFunction;
    WCHAR FileName[PROC_MAX_PATH];
} FSDATA, * PFSDATA;

typedef struct _REGDATA
{
    INT64 SystemTick;
    HANDLE PID;
    LONG NotifyClass;
    WCHAR RegistryFullPath[PROC_MAX_PATH];	//RegistryPath + '\' + RegistryName
} REGDATA, * PREGDATA;

#define BUF_SIZE sizeof(REGDATA)
#define IOCTL_SIZE sizeof(ioCallbackControl)

void StartDriverFunc(int num)
{
    char OutputBuffer[IOCTL_SIZE];
    char InputBuffer[IOCTL_SIZE];

    HANDLE hMapFile;
    PVOID pBuf = NULL;
    HANDLE hDevice;
    size_t ShareSize = 0;
    WCHAR Prefix[1024] = { 0, };
    WCHAR SharedMemoryName[1024] = { 0, };
    WCHAR KernelEventName[1024] = { 0, };
    WCHAR UserEventName[1024] = { 0, };
    WCHAR DeviceName[1024] = { 0, };
    HANDLE KernelEvent;
    HANDLE UserEvent;
    DWORD errNum = 0;
    BOOL bRc;
    PioCallbackControl pIoControl = NULL;
    ULONG bytesReturned;

    switch (num)
    {
    case 0:
        ShareSize = sizeof(OBDATA);
        wcscpy_s(Prefix, L"obprefix");
        wcscpy_s(DeviceName, L"\\\\.\\ProcMonDeviceOB");
        break;
    case 1:
        ShareSize = sizeof(FSDATA);
        wcscpy_s(Prefix, L"fsprefix");
        wcscpy_s(DeviceName, L"\\\\.\\ProcMonDeviceFS");
        break;
    case 2:
        ShareSize = sizeof(REGDATA);
        wcscpy_s(Prefix, L"regprefix");
        wcscpy_s(DeviceName, L"\\\\.\\ProcMonDeviceREG");
        break;
    default:
        break;
    }

    wcscpy_s(SharedMemoryName, PreShareMemory);
    wcscat_s(SharedMemoryName, Prefix);
    wcscat_s(SharedMemoryName, L"SharedMemory");

    wcscpy_s(KernelEventName, PreShareMemory);
    wcscat_s(KernelEventName, Prefix);
    wcscat_s(KernelEventName, L"KernelEvent");

    wcscpy_s(UserEventName, PreShareMemory);
    wcscat_s(UserEventName, Prefix);
    wcscat_s(UserEventName, L"UserEvent");

    KernelEvent = CreateEvent(NULL, FALSE, FALSE, KernelEventName);
    UserEvent = CreateEvent(NULL, FALSE, FALSE, UserEventName);

    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        ShareSize,                // maximum object size (low-order DWORD)
        SharedMemoryName);                 // name of mapping object

    if (hMapFile == NULL)
    {
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),
            GetLastError());
        return;
    }
    pBuf = MapViewOfFile(hMapFile,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        ShareSize);

    if (pBuf == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
            GetLastError());

        CloseHandle(hMapFile);

        return;
    }

    //if (num != 1)
    //{
    //    if ((hDevice = CreateFile(DeviceName,
    //        GENERIC_READ | GENERIC_WRITE,
    //        0,
    //        NULL,
    //        CREATE_ALWAYS,
    //        FILE_ATTRIBUTE_NORMAL,
    //        NULL)) == INVALID_HANDLE_VALUE) {

    //        errNum = GetLastError();

    //        if (errNum != ERROR_FILE_NOT_FOUND) {

    //            printf("CreateFile failed : %d\n", errNum);

    //            return;
    //        }
    //    }

    //    pIoControl = (PioCallbackControl)InputBuffer;

    //    pIoControl->Type = num;
    //    wcscpy_s(pIoControl->CallbackPrefix, 32, Prefix);

    //    bRc = DeviceIoControl(
    //        hDevice,
    //        IOCTL_CALLBACK_START,
    //        &InputBuffer,
    //        sizeof(ioCallbackControl),
    //        &OutputBuffer,
    //        sizeof(ioCallbackControl),
    //        &bytesReturned,
    //        NULL
    //    );
    //    if (!bRc)
    //    {
    //        printf("Error in DeviceIoControl : %d", GetLastError());
    //        return;
    //    }
    //}

    SetEvent(KernelEvent);

    while (true)
    {
        WaitForSingleObject(UserEvent, INFINITE);

        switch (num)
        {
        case 0:
            printf("%p\n", ((POBDATA)pBuf)->PID);
            break;
        case 1:
            printf("FS\n");
            _tprintf(_T("%s\n"), ((PFSDATA)pBuf)->FileName);
            break;
        case 2:
            _tprintf(_T("%s\n"), ((PREGDATA)pBuf)->RegistryFullPath);
            break;
        default:
            break;
        }

        SetEvent(KernelEvent);
    }

    UnmapViewOfFile(pBuf);

    CloseHandle(hMapFile);
}

void DriverExitFunc(int num)
{
    HANDLE hDevice;
    DWORD errNum = 0;
    PioCallbackControl pIoControl = NULL;
    char OutputBuffer[IOCTL_SIZE];
    char InputBuffer[IOCTL_SIZE];
    WCHAR DeviceName[1024];
    BOOL bRc;
    ULONG bytesReturned;

    switch (num)
    {
    case 0:
        wcscpy_s(DeviceName, L"\\\\.\\ProcMonDeviceOB");
        break;
    case 1:
        wcscpy_s(DeviceName, L"\\\\.\\ProcMonDeviceFS");
        break;
    case 2:
        wcscpy_s(DeviceName, L"\\\\.\\ProcMonDeviceREG");
        break;
    default:
        break;
    }

    if ((hDevice = CreateFile(DeviceName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();

        if (errNum != ERROR_FILE_NOT_FOUND) {

            printf("CreateFile failed : %d\n", errNum);

            return;
        }
    }

    pIoControl = (PioCallbackControl)InputBuffer;

    pIoControl->Type = num;

    bRc = DeviceIoControl(
        hDevice,
        IOCTL_CALLBACK_STOP,
        &InputBuffer,
        sizeof(ioCallbackControl),
        &OutputBuffer,
        sizeof(ioCallbackControl),
        &bytesReturned,
        NULL
    );
    if (!bRc)
    {
        printf("Error in DeviceIoControl : %d", GetLastError());
        return;
    }

    CloseHandle(hDevice);
}

int _tmain()
{
    printf("INT64 : %d\n", sizeof(INT64));
    printf("HANDLE : %d\n", sizeof(HANDLE));
    printf("ULONG : %d\n", sizeof(ULONG));
    printf("DWORD : %d\n", sizeof(DWORD));
    printf("UCHAR : %d\n", sizeof(UCHAR));
    printf("WCHAR : %d\n", sizeof(WCHAR));

    printf("obdata : %d\n", sizeof(OBDATA));
    printf("FSdata : %d\n", sizeof(FSDATA));
    printf("regdata : %d\n", sizeof(REGDATA));

    //std::thread t3(StartDriverFunc, 2);
    //std::thread t1(StartDriverFunc, 0);
    //std::thread t2(StartDriverFunc, 1);

    _getch();
    //DriverExitFunc(0);
    //DriverExitFunc(2);
    //DriverExitFunc(1);


    return 0;
}