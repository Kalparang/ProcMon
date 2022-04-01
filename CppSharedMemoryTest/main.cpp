#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define PROC_MAX_PATH 32767

#define PreShareMemory L"Global\\"
#define SharedSectionName L"ProcSharedMemory"
#define ObKernelEventName L"ProcObKernelEvent"
#define ObUserEventName L"ProcObUserEvent"
#define FsKernelEventName L"ProcFsKernelEvent"
#define FsUserEventName L"ProcFsUserEvent"
#define RegKernelEventName L"RegKernelEvent"
#define RegUserEventName L"RegUserEvent"

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

typedef struct IPCStruct
{
    OBDATA ObData;
    FSDATA FsData;
    REGDATA RegData;
} IPCSTRUCT, * PIPCSTRUCT;

#define BUF_SIZE sizeof(IPCSTRUCT)

int _tmain()
{
    HANDLE hMapFile;
    PVOID pBuf;

    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        BUF_SIZE,                // maximum object size (low-order DWORD)
        PreShareMemory SharedSectionName);                 // name of mapping object

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

    PIPCSTRUCT pt = new IPCSTRUCT();
    //WCHAR kernelName[] = PreShareMemory RegKernelEventName;
    //WCHAR userName[] = PreShareMemory RegUserEventName;
    HANDLE kernelSignal;
    HANDLE userSignal;

    //_tprintf(_T("KernelEventName : %s\n"), kernelName);
    //_tprintf(_T("UserEventName : %s\n"), userName);

    CopyMemory(pBuf, pt, sizeof(IPCSTRUCT));
    delete pt;

    pt = (PIPCSTRUCT)pBuf;
    PREGDATA pRegData = &pt->RegData;

    kernelSignal = CreateEvent(NULL, FALSE, FALSE, PreShareMemory RegKernelEventName);
    userSignal = CreateEvent(NULL, FALSE, FALSE, PreShareMemory RegUserEventName);

    SetEvent(kernelSignal);

    while (true)
    {
        WaitForSingleObject(userSignal, INFINITE);
        
        //_tprintf(_T("SystemTick : %lld\n"), pRegData->SystemTick);
        //_tprintf(_T("PID : %ld\n"), pRegData->PID);
        //_tprintf(_T("NotifyClass : %ld\n"), pRegData->NotifyClass);
        //_tprintf(_T("RegistryFullPath : %s\n"), pRegData->RegistryFullPath);

        SetEvent(kernelSignal);
    }

    _getch();

    UnmapViewOfFile(pBuf);

    CloseHandle(hMapFile);

    return 0;
}