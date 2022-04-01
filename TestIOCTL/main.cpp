#include "sioctl.h"

PVOID OutputBuffer[1024];
PVOID InputBuffer[1024];

#define IPC_BUFFER 1024
#define REGSIZE sizeof(REGDATA)
#define REGSTRINGSIZE (IPC_BUFFER - REGSIZE - sizeof(size_t)) / 2

typedef struct _REGDATA
{
    INT64 SystemTick;
    LONG PID;
    LONG NotifyClass;
    PWCH RegistryFullPath;	//RegistryPath + '\' + RegistryName
} REGDATA, * PREGDATA;

typedef struct _REGOUTDATA
{
    size_t RegistryPathLength;
    REGDATA RegData;
    WCHAR RegistryPath[REGSTRINGSIZE];
} REGOUTDATA, * PREGOUTDATA;

VOID __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
)
{
    HANDLE hDevice;
    BOOL bRc;
    ULONG bytesReturned;
    DWORD errNum = 0;
    TCHAR driverLocation[MAX_PATH];

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    if ((hDevice = CreateFile(L"\\\\.\\ProcMonreg",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();

        if (errNum != ERROR_FILE_NOT_FOUND) {

            printf("1CreateFile failed : %d\n", errNum);

            return;
        }

        //
        // The driver is not started yet so let us the install the driver.
        // First setup full path to driver name.
        //

        if (!SetupDriverName(driverLocation, sizeof(driverLocation))) {

            return;
        }

        if (!ManageDriver(DRIVER_NAME,
            driverLocation,
            DRIVER_FUNC_INSTALL
        )) {

            printf("Unable to install driver.\n");

            //
            // Error - remove driver.
            //

            ManageDriver(DRIVER_NAME,
                driverLocation,
                DRIVER_FUNC_REMOVE
            );

            return;
        }

        Sleep(1000);

        hDevice = CreateFile(L"\\\\.\\ProcMonreg",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (hDevice == INVALID_HANDLE_VALUE) {
            printf("2Error: CreatFile Failed : %d\n", GetLastError());
            return;
        }
    }

    printf("InputBuffer Pointer = %p, BufLength = %Iu\n", InputBuffer,
        sizeof(InputBuffer));
    printf("OutputBuffer Pointer = %p BufLength = %Iu\n", OutputBuffer,
        sizeof(OutputBuffer));

    StringCbCopyA((STRSAFE_LPSTR)InputBuffer, sizeof(InputBuffer),
        "This String is from User Application; using METHOD_BUFFERED");
    memset(OutputBuffer, 0, sizeof(OutputBuffer));

    while (true)
    {
        bRc = DeviceIoControl(hDevice,
            (DWORD)2,
            &InputBuffer,
            1024,
            &OutputBuffer,
            1024,
            &bytesReturned,
            NULL
        );

        if (!bRc)
        {
            printf("Error in DeviceIoControl : %d", GetLastError());
            //return;
        }

        wprintf_s(L"bytesReturned : %d\n", bytesReturned);

        PREGOUTDATA pRegOutData = (PREGOUTDATA)OutputBuffer;

        size_t RegistryPathLength = pRegOutData->RegistryPathLength;
        REGDATA regData = pRegOutData->RegData;
        wchar_t* RegistryPath = pRegOutData->RegistryPath;

        wprintf_s(L"RegistryPathLength : %llu\n", RegistryPathLength);
        wprintf_s(L"RegistryPath : %s\n", RegistryPath);
        wprintf_s(L"SystemTick : %d\n", regData.SystemTick);
        wprintf_s(L"PID : %d\n", regData.PID);
        wprintf_s(L"PID : %d\n\n", regData.NotifyClass);
    }
}