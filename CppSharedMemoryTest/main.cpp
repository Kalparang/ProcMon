#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>

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
    LONG PID;
    LONG TargetPID;
    ULONG Operation;
    ACCESS_MASK DesiredAccess;
} OBDATA, * POBDATA;

typedef struct _FSDATA
{
    INT64 SystemTick;
    LONG PID;
    UCHAR MajorFunction;
    WCHAR FileName[PROC_MAX_PATH];
} FSDATA, * PFSDATA;

typedef struct _REGDATA
{
    INT64 SystemTick;
    LONG PID;
    LONG NotifyClass;
    WCHAR RegistryFullPath[PROC_MAX_PATH];	//RegistryPath + '\' + RegistryName
} REGDATA, * PREGDATA;

#define BUF_SIZE sizeof(REGDATA)
#define IOCTL_SIZE sizeof(ioCallbackControl)

char OutputBuffer[IOCTL_SIZE];
char InputBuffer[IOCTL_SIZE];

int _tmain()
{
    HANDLE hMapFile;
    PVOID pBuf;
    HANDLE hDevice;
    BOOL bRc;
    ULONG bytesReturned;
    DWORD errNum = 0;
    PioCallbackControl pIoControl = NULL;
    WCHAR Prefix[] = L"regprefix";
    WCHAR SharedMemoryName[1024] = { 0 };
    WCHAR KernelEventName[1024] = { 0 };
    WCHAR UserEventName[1024] = { 0 };
    PREGDATA pRegData = NULL;
    HANDLE KernelEvent;
    HANDLE UserEvent;

    wcscpy_s(SharedMemoryName, PreShareMemory);
    wcscat_s(SharedMemoryName, Prefix);
    wcscat_s(SharedMemoryName, L"SharedMemory");

    wcscpy_s(KernelEventName, PreShareMemory);
    wcscat_s(KernelEventName, Prefix);
    wcscat_s(KernelEventName, L"KernelEvent");

    wcscpy_s(UserEventName, PreShareMemory);
    wcscat_s(UserEventName, Prefix);
    wcscat_s(UserEventName, L"UserEvent");

    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        BUF_SIZE,                // maximum object size (low-order DWORD)
        SharedMemoryName);                 // name of mapping object

    if (hMapFile == NULL)
    {
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),
            GetLastError());
        return 1;
    }
    pBuf = MapViewOfFile(hMapFile,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        BUF_SIZE);

    if (pBuf == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
            GetLastError());

        CloseHandle(hMapFile);

        return 1;
    }

    _tprintf(_T("SharedMemory : %s\n"), SharedMemoryName);
    _tprintf(_T("KernelEvent : %s\n"), KernelEventName);
    _tprintf(_T("UserEvent : %s\n"), UserEventName);

    KernelEvent = CreateEvent(NULL, FALSE, FALSE, KernelEventName);
    UserEvent = CreateEvent(NULL, FALSE, FALSE, UserEventName);

    pRegData = new REGDATA();
    printf("InputBuffer Pointer = %p, BufLength = %Iu\n", InputBuffer,
        sizeof(InputBuffer));
    printf("OutputBuffer Pointer = %p BufLength = %Iu\n", OutputBuffer,
        sizeof(OutputBuffer));

    pRegData->NotifyClass = 1;
    pRegData->PID = 2;
    pRegData->SystemTick = 3;
    wcscpy_s(pRegData->RegistryFullPath, L"Registry Full Path");

    memcpy(pBuf, pRegData, sizeof(REGDATA));
    delete pRegData;

    printf("SharedMemory Create\n");
    printf("Wait IOCTL\n");
    _getch();

    if ((hDevice = CreateFile(L"\\\\.\\ProcMonDevice",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();

        if (errNum != ERROR_FILE_NOT_FOUND) {

            printf("CreateFile failed : %d\n", errNum);

            return 0;
        }
    }

    pIoControl = (PioCallbackControl)InputBuffer;

    pIoControl->Type = 2;
    wcscpy_s(pIoControl->CallbackPrefix, 32, Prefix);

    bRc = DeviceIoControl(
        hDevice,
        IOCTL_CALLBACK_START,
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
        return 0;
    }

    printf("set kernel\n");
    _getch();

    SetEvent(KernelEvent);
    pRegData = (PREGDATA)pBuf;

    unsigned long long Num = 0;

    while (true)
    {
        WaitForSingleObject(UserEvent, INFINITE);

        //memcpy(pRegData, pBuf, sizeof(REGDATA));

        _tprintf(_T("%lld\n"), pRegData->SystemTick);

        //_tprintf(_T("SystemTick : %lld\n"), pRegData->SystemTick);
        //_tprintf(_T("PID : %ld\n"), pRegData->PID);
        //_tprintf(_T("NotifyClass : %ld\n"), pRegData->NotifyClass);
        //_tprintf(_T("RegistryFullPath : %s\n"), pRegData->RegistryFullPath);

        //_getch();

        SetEvent(KernelEvent);
    }

    _getch();

    UnmapViewOfFile(pBuf);

    CloseHandle(hMapFile);

    return 0;
}