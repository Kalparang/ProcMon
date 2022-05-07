NTSTATUS
ProcMonDriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

VOID
SetDriverName(
    PWCH NT_NAME,
    PWCH DOS_NAME
);

VOID
SetOBName(
    PWCH NT_NAME,
    PWCH DOS_NAME
);

VOID
SetFSName(
    PWCH NT_NAME,
    PWCH DOS_NAME
);

VOID
SetREGName(
    PWCH NT_NAME,
    PWCH DOS_NAME
);