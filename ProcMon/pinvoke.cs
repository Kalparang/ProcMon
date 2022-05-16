using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace ProcMon
{
    public class pinvoke
    {
        [DllImport("advapi32.dll", EntryPoint = "OpenSCManagerW", ExactSpelling = true, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr OpenSCManager(
            string machineName,
            string databaseName,
            SCM_ACCESS dwAccess
            );

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern IntPtr CreateService(
            IntPtr hSCManager,
            string lpServiceName,
            string lpDisplayName,
            SERVICE_ACCESS dwDesiredAccess,
            SERVICE_TYPE dwServiceType,
            SERVICE_START dwStartType,
            SERVICE_ERROR dwErrorControl,
            string lpBinaryPathName,
            [Optional] string lpLoadOrderGroup,
            [Optional] string lpdwTagId,    // only string so we can pass null
            [Optional] string lpDependencies,
            [Optional] string lpServiceStartName,
            [Optional] string lpPassword
            );

        [DllImport("advapi32.dll", EntryPoint = "OpenServiceA", SetLastError = true, CharSet = CharSet.Ansi)]
        public static extern IntPtr OpenService(
            IntPtr hSCManager,
            string lpServiceName,
            SERVICE_ACCESS dwDesiredAccess
            );

        [DllImport("advapi32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ControlService(
            IntPtr hService,
            SERVICE_CONTROL dwControl, 
            ref SERVICE_STATUS lpServiceStatus
            );

        [DllImport("advapi32", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool StartService(
                IntPtr hService,
                int dwNumServiceArgs,
                string[] lpServiceArgVectors
            );

        [DllImport("advapi32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DeleteService(IntPtr hService);

        [DllImport("advapi32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseServiceHandle(IntPtr hSCObject);

        [StructLayout(LayoutKind.Sequential, Pack = 0)]
        public struct SERVICE_STATUS
        {
            public SERVICE_TYPE dwServiceType;
            public SERVICE_STATE dwCurrentState;
            public SERVICE_ACCEPT dwControlsAccepted;
            public uint dwWin32ExitCode;
            public uint dwServiceSpecificExitCode;
            public uint dwCheckPoint;
            public uint dwWaitHint;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 0)]
        public struct OBDATA
        {
            public Int64 SystemTick;
            public long PID;
            public long TargetPID;
            public UInt32 Operation;
            public UInt32 DesiredAccess;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 0, CharSet = CharSet.Unicode)]
        public struct FSDATA
        {
            public Int64 SystemTick;
            public long PID;
            public IRP_MAJORFUNCTION MajorFunction;
            public IO_STACK_LOCATION_FLAGS Flag;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32767)]
            public string FileName;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 0, CharSet = CharSet.Unicode)]
        public struct REGDATA
        {
            public Int64 SystemTick;
            public long PID;
            public REG_NOTIFY_CLASS NotifyClass;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32767)]
            public string RegistryFullPath;
        }

        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool SetForegroundWindow(IntPtr handle);
        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool ShowWindow(IntPtr handle, CMD_SHOW nCmdShow);
        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool IsIconic(IntPtr handle);

        public const string OBPrefix = "obprefix";
        public const string FSPrefix = "fsprefix";
        public const string REGPrefix = "regprefix";

        public const string OBSysName = "ProcMonOb.sys";
        public const string FSSysName = "ProcMonFS.sys";
        public const string REGSysName = "ProcMonREG.sys";

        public const string OBServiceName = "ProcMonOb";
        public const string FSServiceName = "ProcMonFS";
        public const string REGServiceName = "ProcMonREG";

        public const string PreShareMemory = "Global\\";

        [Flags]
        public enum ACCESS_MASK : uint
        {
            DELETE = 0x00010000,
            READ_CONTROL = 0x00020000,
            WRITE_DAC = 0x00040000,
            WRITE_OWNER = 0x00080000,
            SYNCHRONIZE = 0x00100000,

            STANDARD_RIGHTS_REQUIRED = 0x000F0000,

            STANDARD_RIGHTS_READ = 0x00020000,
            STANDARD_RIGHTS_WRITE = 0x00020000,
            STANDARD_RIGHTS_EXECUTE = 0x00020000,

            STANDARD_RIGHTS_ALL = 0x001F0000,

            SPECIFIC_RIGHTS_ALL = 0x0000FFFF,

            ACCESS_SYSTEM_SECURITY = 0x01000000,

            MAXIMUM_ALLOWED = 0x02000000,

            GENERIC_READ = 0x80000000,
            GENERIC_WRITE = 0x40000000,
            GENERIC_EXECUTE = 0x20000000,
            GENERIC_ALL = 0x10000000,

            DESKTOP_READOBJECTS = 0x00000001,
            DESKTOP_CREATEWINDOW = 0x00000002,
            DESKTOP_CREATEMENU = 0x00000004,
            DESKTOP_HOOKCONTROL = 0x00000008,
            DESKTOP_JOURNALRECORD = 0x00000010,
            DESKTOP_JOURNALPLAYBACK = 0x00000020,
            DESKTOP_ENUMERATE = 0x00000040,
            DESKTOP_WRITEOBJECTS = 0x00000080,
            DESKTOP_SWITCHDESKTOP = 0x00000100,

            WINSTA_ENUMDESKTOPS = 0x00000001,
            WINSTA_READATTRIBUTES = 0x00000002,
            WINSTA_ACCESSCLIPBOARD = 0x00000004,
            WINSTA_CREATEDESKTOP = 0x00000008,
            WINSTA_WRITEATTRIBUTES = 0x00000010,
            WINSTA_ACCESSGLOBALATOMS = 0x00000020,
            WINSTA_EXITWINDOWS = 0x00000040,
            WINSTA_ENUMERATE = 0x00000100,
            WINSTA_READSCREEN = 0x00000200,

            WINSTA_ALL_ACCESS = 0x0000037F
        }

        [Flags]
        public enum SCM_ACCESS : uint
        {
            /// <summary>
            /// Required to connect to the service control manager.
            /// </summary>
            SC_MANAGER_CONNECT = 0x00001,

            /// <summary>
            /// Required to call the CreateService function to create a service
            /// object and add it to the database.
            /// </summary>
            SC_MANAGER_CREATE_SERVICE = 0x00002,

            /// <summary>
            /// Required to call the EnumServicesStatusEx function to list the
            /// services that are in the database.
            /// </summary>
            SC_MANAGER_ENUMERATE_SERVICE = 0x00004,

            /// <summary>
            /// Required to call the LockServiceDatabase function to acquire a
            /// lock on the database.
            /// </summary>
            SC_MANAGER_LOCK = 0x00008,

            /// <summary>
            /// Required to call the QueryServiceLockStatus function to retrieve
            /// the lock status information for the database.
            /// </summary>
            SC_MANAGER_QUERY_LOCK_STATUS = 0x00010,

            /// <summary>
            /// Required to call the NotifyBootConfigStatus function.
            /// </summary>
            SC_MANAGER_MODIFY_BOOT_CONFIG = 0x00020,

            /// <summary>
            /// Includes STANDARD_RIGHTS_REQUIRED, in addition to all access
            /// rights in this table.
            /// </summary>
            SC_MANAGER_ALL_ACCESS = ACCESS_MASK.STANDARD_RIGHTS_REQUIRED |
                SC_MANAGER_CONNECT |
                SC_MANAGER_CREATE_SERVICE |
                SC_MANAGER_ENUMERATE_SERVICE |
                SC_MANAGER_LOCK |
                SC_MANAGER_QUERY_LOCK_STATUS |
                SC_MANAGER_MODIFY_BOOT_CONFIG,

            GENERIC_READ = ACCESS_MASK.STANDARD_RIGHTS_READ |
                SC_MANAGER_ENUMERATE_SERVICE |
                SC_MANAGER_QUERY_LOCK_STATUS,

            GENERIC_WRITE = ACCESS_MASK.STANDARD_RIGHTS_WRITE |
                SC_MANAGER_CREATE_SERVICE |
                SC_MANAGER_MODIFY_BOOT_CONFIG,

            GENERIC_EXECUTE = ACCESS_MASK.STANDARD_RIGHTS_EXECUTE |
                SC_MANAGER_CONNECT | SC_MANAGER_LOCK,

            GENERIC_ALL = SC_MANAGER_ALL_ACCESS,
        }

        /// <summary>
        /// Access to the service. Before granting the requested access, the
        /// system checks the access token of the calling process.
        /// </summary>
        [Flags]
        public enum SERVICE_ACCESS : uint
        {
            /// <summary>
            /// Required to call the QueryServiceConfig and
            /// QueryServiceConfig2 functions to query the service configuration.
            /// </summary>
            SERVICE_QUERY_CONFIG = 0x00001,

            /// <summary>
            /// Required to call the ChangeServiceConfig or ChangeServiceConfig2 function
            /// to change the service configuration. Because this grants the caller
            /// the right to change the executable file that the system runs,
            /// it should be granted only to administrators.
            /// </summary>
            SERVICE_CHANGE_CONFIG = 0x00002,

            /// <summary>
            /// Required to call the QueryServiceStatusEx function to ask the service
            /// control manager about the status of the service.
            /// </summary>
            SERVICE_QUERY_STATUS = 0x00004,

            /// <summary>
            /// Required to call the EnumDependentServices function to enumerate all
            /// the services dependent on the service.
            /// </summary>
            SERVICE_ENUMERATE_DEPENDENTS = 0x00008,

            /// <summary>
            /// Required to call the StartService function to start the service.
            /// </summary>
            SERVICE_START = 0x00010,

            /// <summary>
            ///     Required to call the ControlService function to stop the service.
            /// </summary>
            SERVICE_STOP = 0x00020,

            /// <summary>
            /// Required to call the ControlService function to pause or continue
            /// the service.
            /// </summary>
            SERVICE_PAUSE_CONTINUE = 0x00040,

            /// <summary>
            /// Required to call the EnumDependentServices function to enumerate all
            /// the services dependent on the service.
            /// </summary>
            SERVICE_INTERROGATE = 0x00080,

            /// <summary>
            /// Required to call the ControlService function to specify a user-defined
            /// control code.
            /// </summary>
            SERVICE_USER_DEFINED_CONTROL = 0x00100,

            /// <summary>
            /// Includes STANDARD_RIGHTS_REQUIRED in addition to all access rights in this table.
            /// </summary>
            SERVICE_ALL_ACCESS = (ACCESS_MASK.STANDARD_RIGHTS_REQUIRED |
                SERVICE_QUERY_CONFIG |
                SERVICE_CHANGE_CONFIG |
                SERVICE_QUERY_STATUS |
                SERVICE_ENUMERATE_DEPENDENTS |
                SERVICE_START |
                SERVICE_STOP |
                SERVICE_PAUSE_CONTINUE |
                SERVICE_INTERROGATE |
                SERVICE_USER_DEFINED_CONTROL),

            GENERIC_READ = ACCESS_MASK.STANDARD_RIGHTS_READ |
                SERVICE_QUERY_CONFIG |
                SERVICE_QUERY_STATUS |
                SERVICE_INTERROGATE |
                SERVICE_ENUMERATE_DEPENDENTS,

            GENERIC_WRITE = ACCESS_MASK.STANDARD_RIGHTS_WRITE |
                SERVICE_CHANGE_CONFIG,

            GENERIC_EXECUTE = ACCESS_MASK.STANDARD_RIGHTS_EXECUTE |
                SERVICE_START |
                SERVICE_STOP |
                SERVICE_PAUSE_CONTINUE |
                SERVICE_USER_DEFINED_CONTROL,

            /// <summary>
            /// Required to call the QueryServiceObjectSecurity or
            /// SetServiceObjectSecurity function to access the SACL. The proper
            /// way to obtain this access is to enable the SE_SECURITY_NAME
            /// privilege in the caller's current access token, open the handle
            /// for ACCESS_SYSTEM_SECURITY access, and then disable the privilege.
            /// </summary>
            ACCESS_SYSTEM_SECURITY = ACCESS_MASK.ACCESS_SYSTEM_SECURITY,

            /// <summary>
            /// Required to call the DeleteService function to delete the service.
            /// </summary>
            DELETE = ACCESS_MASK.DELETE,

            /// <summary>
            /// Required to call the QueryServiceObjectSecurity function to query
            /// the security descriptor of the service object.
            /// </summary>
            READ_CONTROL = ACCESS_MASK.READ_CONTROL,

            /// <summary>
            /// Required to call the SetServiceObjectSecurity function to modify
            /// the Dacl member of the service object's security descriptor.
            /// </summary>
            WRITE_DAC = ACCESS_MASK.WRITE_DAC,

            /// <summary>
            /// Required to call the SetServiceObjectSecurity function to modify
            /// the Owner and Group members of the service object's security
            /// descriptor.
            /// </summary>
            WRITE_OWNER = ACCESS_MASK.WRITE_OWNER,
        }

        /// <summary>
        /// Service types.
        /// </summary>
        [Flags]
        public enum SERVICE_TYPE : uint
        {
            /// <summary>
            /// Driver service.
            /// </summary>
            SERVICE_KERNEL_DRIVER = 0x00000001,

            /// <summary>
            /// File system driver service.
            /// </summary>
            SERVICE_FILE_SYSTEM_DRIVER = 0x00000002,

            /// <summary>
            /// Service that runs in its own process.
            /// </summary>
            SERVICE_WIN32_OWN_PROCESS = 0x00000010,

            /// <summary>
            /// Service that shares a process with one or more other services.
            /// </summary>
            SERVICE_WIN32_SHARE_PROCESS = 0x00000020,

            /// <summary>
            /// The service can interact with the desktop.
            /// </summary>
            SERVICE_INTERACTIVE_PROCESS = 0x00000100,
        }

        /// <summary>
        /// Service start options
        /// </summary>
        public enum SERVICE_START : uint
        {
            /// <summary>
            /// A device driver started by the system loader. This value is valid
            /// only for driver services.
            /// </summary>
            SERVICE_BOOT_START = 0x00000000,

            /// <summary>
            /// A device driver started by the IoInitSystem function. This value
            /// is valid only for driver services.
            /// </summary>
            SERVICE_SYSTEM_START = 0x00000001,

            /// <summary>
            /// A service started automatically by the service control manager
            /// during system startup. For more information, see Automatically
            /// Starting Services.
            /// </summary>        
            SERVICE_AUTO_START = 0x00000002,

            /// <summary>
            /// A service started by the service control manager when a process
            /// calls the StartService function. For more information, see
            /// Starting Services on Demand.
            /// </summary>
            SERVICE_DEMAND_START = 0x00000003,

            /// <summary>
            /// A service that cannot be started. Attempts to start the service
            /// result in the error code ERROR_SERVICE_DISABLED.
            /// </summary>
            SERVICE_DISABLED = 0x00000004,
        }

        /// <summary>
        /// Severity of the error, and action taken, if this service fails
        /// to start.
        /// </summary>
        public enum SERVICE_ERROR
        {
            /// <summary>
            /// The startup program ignores the error and continues the startup
            /// operation.
            /// </summary>
            SERVICE_ERROR_IGNORE = 0x00000000,

            /// <summary>
            /// The startup program logs the error in the event log but continues
            /// the startup operation.
            /// </summary>
            SERVICE_ERROR_NORMAL = 0x00000001,

            /// <summary>
            /// The startup program logs the error in the event log. If the
            /// last-known-good configuration is being started, the startup
            /// operation continues. Otherwise, the system is restarted with
            /// the last-known-good configuration.
            /// </summary>
            SERVICE_ERROR_SEVERE = 0x00000002,

            /// <summary>
            /// The startup program logs the error in the event log, if possible.
            /// If the last-known-good configuration is being started, the startup
            /// operation fails. Otherwise, the system is restarted with the
            /// last-known good configuration.
            /// </summary>
            SERVICE_ERROR_CRITICAL = 0x00000003,
        }

        [Flags]
        public enum SERVICE_CONTROL : uint
        {
            STOP = 0x00000001,
            PAUSE = 0x00000002,
            CONTINUE = 0x00000003,
            INTERROGATE = 0x00000004,
            SHUTDOWN = 0x00000005,
            PARAMCHANGE = 0x00000006,
            NETBINDADD = 0x00000007,
            NETBINDREMOVE = 0x00000008,
            NETBINDENABLE = 0x00000009,
            NETBINDDISABLE = 0x0000000A,
            DEVICEEVENT = 0x0000000B,
            HARDWAREPROFILECHANGE = 0x0000000C,
            POWEREVENT = 0x0000000D,
            SESSIONCHANGE = 0x0000000E
        }

        public enum SERVICE_STATE : uint
        {
            SERVICE_STOPPED = 0x00000001,
            SERVICE_START_PENDING = 0x00000002,
            SERVICE_STOP_PENDING = 0x00000003,
            SERVICE_RUNNING = 0x00000004,
            SERVICE_CONTINUE_PENDING = 0x00000005,
            SERVICE_PAUSE_PENDING = 0x00000006,
            SERVICE_PAUSED = 0x00000007
        }

        [Flags]
        public enum SERVICE_ACCEPT : uint
        {
            STOP = 0x00000001,
            PAUSE_CONTINUE = 0x00000002,
            SHUTDOWN = 0x00000004,
            PARAMCHANGE = 0x00000008,
            NETBINDCHANGE = 0x00000010,
            HARDWAREPROFILECHANGE = 0x00000020,
            POWEREVENT = 0x00000040,
            SESSIONCHANGE = 0x00000080,
        }

        /// <summary>
        /// FileSystem에서 쓰임
        /// Create부터 Write까지만 쓰임
        /// </summary>
        public enum IRP_MAJORFUNCTION : byte
        {
            IRP_MJ_CREATE = 0x00,
            IRP_MJ_CREATE_NAMED_PIPE = 0x01,
            IRP_MJ_CLOSE = 0x02,
            IRP_MJ_READ = 0x03,
            IRP_MJ_WRITE = 0x04,
            IRP_MJ_QUERY_INFORMATION = 0x05,
            IRP_MJ_SET_INFORMATION = 0x06,
            IRP_MJ_QUERY_EA = 0x07,
            IRP_MJ_SET_EA = 0x08,
            IRP_MJ_FLUSH_BUFFERS = 0x09,
            IRP_MJ_QUERY_VOLUME_INFORMATION = 0x0a,
            IRP_MJ_SET_VOLUME_INFORMATION = 0x0b,
            IRP_MJ_DIRECTORY_CONTROL = 0x0c,
            IRP_MJ_FILE_SYSTEM_CONTROL = 0x0d,
            IRP_MJ_DEVICE_CONTROL = 0x0e,
            IRP_MJ_INTERNAL_DEVICE_CONTROL = 0x0f,
            IRP_MJ_SHUTDOWN = 0x10,
            IRP_MJ_LOCK_CONTROL = 0x11,
            IRP_MJ_CLEANUP = 0x12,
            IRP_MJ_CREATE_MAILSLOT = 0x13,
            IRP_MJ_QUERY_SECURITY = 0x14,
            IRP_MJ_SET_SECURITY = 0x15,
            IRP_MJ_POWER = 0x16,
            IRP_MJ_SYSTEM_CONTROL = 0x17,
            IRP_MJ_DEVICE_CHANGE = 0x18,
            IRP_MJ_QUERY_QUOTA = 0x19,
            IRP_MJ_SET_QUOTA = 0x1a,
            IRP_MJ_PNP = 0x1b,
            IRP_MJ_PNP_POWER = IRP_MJ_PNP,
            IRP_MJ_MAXIMUM_FUNCTION = 0x1b
        }

        public enum IO_STACK_LOCATION_FLAGS : byte
        {
            SL_NORMAL = 0x00,
            SL_KEY_SPECIFIED = 0x01,
            SL_OVERRIDE_VERIFY_VOLUME = 0x02,
            SL_WRITE_THROUGH = 0x04,
            SL_FT_SEQUENTIAL_WRITE = 0x08,
            SL_FORCE_DIRECT_WRITE = 0x10,
            SL_REALTIME_STREAM = 0x20,
            SL_PERSISTENT_MEMORY_FIXED_MAPPING = 0x20
        }

        public enum REG_NOTIFY_CLASS : int
        {
            RegNtDeleteKey,
            RegNtPreDeleteKey = RegNtDeleteKey,
            RegNtSetValueKey,
            RegNtPreSetValueKey = RegNtSetValueKey,
            RegNtDeleteValueKey,
            RegNtPreDeleteValueKey = RegNtDeleteValueKey,
            RegNtSetInformationKey,
            RegNtPreSetInformationKey = RegNtSetInformationKey,
            RegNtRenameKey,
            RegNtPreRenameKey = RegNtRenameKey,
            RegNtEnumerateKey,
            RegNtPreEnumerateKey = RegNtEnumerateKey,
            RegNtEnumerateValueKey,
            RegNtPreEnumerateValueKey = RegNtEnumerateValueKey,
            RegNtQueryKey,
            RegNtPreQueryKey = RegNtQueryKey,
            RegNtQueryValueKey,
            RegNtPreQueryValueKey = RegNtQueryValueKey,
            RegNtQueryMultipleValueKey,
            RegNtPreQueryMultipleValueKey = RegNtQueryMultipleValueKey,
            RegNtPreCreateKey,
            RegNtPostCreateKey,
            RegNtPreOpenKey,
            RegNtPostOpenKey,
            RegNtKeyHandleClose,
            RegNtPreKeyHandleClose = RegNtKeyHandleClose,
            //
            // .Net only
            //    
            RegNtPostDeleteKey,
            RegNtPostSetValueKey,
            RegNtPostDeleteValueKey,
            RegNtPostSetInformationKey,
            RegNtPostRenameKey,
            RegNtPostEnumerateKey,
            RegNtPostEnumerateValueKey,
            RegNtPostQueryKey,
            RegNtPostQueryValueKey,
            RegNtPostQueryMultipleValueKey,
            RegNtPostKeyHandleClose,
            RegNtPreCreateKeyEx,
            RegNtPostCreateKeyEx,
            RegNtPreOpenKeyEx,
            RegNtPostOpenKeyEx,
            //
            // new to Windows Vista
            //
            RegNtPreFlushKey,
            RegNtPostFlushKey,
            RegNtPreLoadKey,
            RegNtPostLoadKey,
            RegNtPreUnLoadKey,
            RegNtPostUnLoadKey,
            RegNtPreQueryKeySecurity,
            RegNtPostQueryKeySecurity,
            RegNtPreSetKeySecurity,
            RegNtPostSetKeySecurity,
            //
            // per-object context cleanup
            //
            RegNtCallbackObjectContextCleanup,
            //
            // new in Vista SP2 
            //
            RegNtPreRestoreKey,
            RegNtPostRestoreKey,
            RegNtPreSaveKey,
            RegNtPostSaveKey,
            RegNtPreReplaceKey,
            RegNtPostReplaceKey,
            //
            // new to Windows 10
            //
            RegNtPreQueryKeyName,
            RegNtPostQueryKeyName,

            MaxRegNtNotifyClass //should always be the last enum
        }

        public enum CMD_SHOW : int
        {
            SW_HIDE,
            SW_SHOWNORMAL,
            SW_NORMAL = SW_SHOWNORMAL,
            SW_SHOWMINIMIZED,
            SW_SHOWMAXIMIZED,
            SW_MAXIMIZE = SW_SHOWMAXIMIZED,
            SW_SHOWNOACTIVATE,
            SW_SHOW,
            SW_MINIMIZE,
            SW_SHOWMINNOACTIVE,
            SW_SHOWNA,
            SW_RESTORE,
            SW_SHOWDEFAULT,
            SW_FORCEMINIMIZE
        }
        public enum DRIVER_TYPE : int
        {
            OB,
            FILESYSTEM,
            REGISTRY
        }

        public static void BringProcessToFront(long PID)
        {
            try
            {
                IntPtr handle = Process.GetProcessById((int)PID).MainWindowHandle;
                if (IsIconic(handle))
                {
                    ShowWindow(handle, CMD_SHOW.SW_RESTORE);
                }

                SetForegroundWindow(handle);
            }
            catch(Exception e)
            {

            }
        }
    }
}
