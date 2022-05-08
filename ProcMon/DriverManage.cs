using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ServiceProcess;
using System.Runtime.InteropServices;
using System.Reflection;
using System.IO.MemoryMappedFiles;
using System.Threading;

namespace ProcMon
{
    public class DriverManage
    {
        public static int CreateDriverServices()
        {
            var SCManager = OpenSCManager();
            if(SCManager == IntPtr.Zero)
            {
                var err = Marshal.GetLastWin32Error();
                Console.WriteLine(MethodBase.GetCurrentMethod().Name + " err : " + err);
                return err;
            }

            for (int i = 0; i < 3; i++)
            {
                var Service = CreateDriverService(SCManager, i);
                pinvoke.CloseServiceHandle(Service);
            }

            pinvoke.CloseServiceHandle(SCManager);

            return 0;
        }

        public static int DeleteServices()
        {
            var SCManager = OpenSCManager();
            if (SCManager == IntPtr.Zero)
            {
                var err = Marshal.GetLastWin32Error();
                Console.WriteLine(MethodBase.GetCurrentMethod().Name + " err : " + err);
                return err;
            }

            for (int i = 0; i < 3; i++)
            {
                var Service = OpenDriverSerivce(SCManager, i);
                pinvoke.SERVICE_STATUS ss = new pinvoke.SERVICE_STATUS();

                pinvoke.ControlService(Service, pinvoke.SERVICE_CONTROL.STOP, ref ss);
                pinvoke.DeleteService(Service);
                pinvoke.CloseServiceHandle(Service);
            }

            pinvoke.CloseServiceHandle(SCManager);

            return 0;
        }

        public static int StartService(int Type)
        {
            var SCManager = OpenSCManager();
            if (SCManager == IntPtr.Zero)
            {
                var err = Marshal.GetLastWin32Error();
                Console.WriteLine(MethodBase.GetCurrentMethod().Name + " err : " + err);
                return err;
            }

            var Service = OpenDriverSerivce(SCManager, Type);
            pinvoke.StartService(Service, 0, null);

            pinvoke.CloseServiceHandle(Service);
            pinvoke.CloseServiceHandle(SCManager);

            return 0;
        }

        public static bool StopService(int Type)
        {
            var SCManager = OpenSCManager();
            if (SCManager == IntPtr.Zero)
            {
                var err = Marshal.GetLastWin32Error();
                Console.WriteLine(MethodBase.GetCurrentMethod().Name + " err : " + err);
                return false;
            }

            var Service = OpenDriverSerivce(SCManager, Type);
            pinvoke.SERVICE_STATUS ss = new pinvoke.SERVICE_STATUS();
            pinvoke.ControlService(Service, pinvoke.SERVICE_CONTROL.STOP, ref ss);

            pinvoke.CloseServiceHandle(Service);
            pinvoke.CloseServiceHandle(SCManager);

            return true;
        }

        static IntPtr OpenSCManager()
        {
            var SCManager = pinvoke.OpenSCManager(
                null, null, pinvoke.SCM_ACCESS.SC_MANAGER_ALL_ACCESS);

            return SCManager;
        }

        static IntPtr CreateDriverService(IntPtr SCManager, int Type)
        {
            string ServiceName;
            string DisplayName;
            string PathName;

            switch (Type)
            {
                case 0:
                    ServiceName = pinvoke.OBServiceName;
                    DisplayName = pinvoke.OBServiceName;
                    PathName = Environment.CurrentDirectory + @"\" + pinvoke.OBSysName;
                    break;
                case 1:
                    ServiceName = pinvoke.FSServiceName;
                    DisplayName = pinvoke.FSServiceName;
                    PathName = Environment.CurrentDirectory + @"\" + pinvoke.FSSysName;
                    break;
                case 2:
                    ServiceName = pinvoke.REGServiceName;
                    DisplayName = pinvoke.REGServiceName;
                    PathName = Environment.CurrentDirectory + @"\" + pinvoke.REGSysName;
                    break;
                default:
                    return IntPtr.Zero;
            }

            var Service = pinvoke.CreateService(
                SCManager,
                ServiceName, DisplayName,
                pinvoke.SERVICE_ACCESS.SERVICE_ALL_ACCESS,
                pinvoke.SERVICE_TYPE.SERVICE_KERNEL_DRIVER,
                pinvoke.SERVICE_START.SERVICE_DEMAND_START,
                pinvoke.SERVICE_ERROR.SERVICE_ERROR_NORMAL,
                PathName,
                null, null, null, null, null
                );

            return Service;
        }

        static IntPtr OpenDriverSerivce(IntPtr SCManager, int Type)
        {
            string ServiceName;

            switch (Type)
            {
                case 0:
                    ServiceName = pinvoke.OBServiceName;
                    break;
                case 1:
                    ServiceName = pinvoke.FSServiceName;
                    break;
                case 2:
                    ServiceName = pinvoke.REGServiceName;
                    break;
                default:
                    return IntPtr.Zero;
            }

            var Service = pinvoke.OpenService(
                SCManager,
                ServiceName, pinvoke.SERVICE_ACCESS.SERVICE_ALL_ACCESS
                );

            return Service;
        }
    }

    public class IPC
    {
        MemoryMappedFile mm = null;
        object targetData = null;
        EventWaitHandle kernelEvent = null;
        EventWaitHandle userEvent = null;
        int Type = 0;
        long capacity;

        public void Init(int Type)
        {
            this.Type = Type;

            CreateDriverEvent();

            if (kernelEvent == null
                || userEvent == null)
            {
                Console.WriteLine("CreateDriverEvent fail");
                return;
            }

            CreateSharedMemory();
            if (mm == null)
            {
                Console.WriteLine("CreateSharedMemory fail");
                return;
            }

            new Thread(new ThreadStart(test)).Start();
        }

        MemoryMappedFile CreateSharedMemory()
        {
            string mapName = pinvoke.PreShareMemory;

            switch (Type)
            {
                case 0:
                    mapName += pinvoke.OBPrefix;
                    targetData = new pinvoke.OBDATA();
                    capacity = Marshal.SizeOf(targetData);
                    break;
                case 1:
                    mapName += pinvoke.FSPrefix;
                    targetData = new pinvoke.FSDATA();
                    capacity = Marshal.SizeOf(targetData);
                    break;
                case 2:
                    mapName += pinvoke.REGPrefix;
                    targetData = new pinvoke.REGDATA();
                    capacity = Marshal.SizeOf(targetData);
                    break;
                default:
                    targetData = null;
                    return null;
            }
            mapName += "SharedMemory";

            mm = MemoryMappedFile.CreateNew(mapName, capacity, MemoryMappedFileAccess.ReadWrite);

            return mm;
        }

        void CreateDriverEvent()
        {
            string eventName = pinvoke.PreShareMemory;

            switch (Type)
            {
                case 0:
                    eventName += pinvoke.OBPrefix;
                    break;
                case 1:
                    eventName += pinvoke.FSPrefix;
                    break;
                case 2:
                    eventName += pinvoke.REGPrefix;
                    break;
                default:
                    return;
            }

            kernelEvent = new EventWaitHandle(false, EventResetMode.AutoReset, eventName + "KernelEvent");
            userEvent = new EventWaitHandle(false, EventResetMode.AutoReset, eventName + "UserEvent");
        }

        void test()
        {
            Console.WriteLine("Thread start");

            byte[] data = new byte[capacity];
            IntPtr p = Marshal.AllocHGlobal((int)capacity);

            kernelEvent.Set();

            using (var accessor = mm.CreateViewAccessor())
            {
                while (true)
                {
                    userEvent.WaitOne();

                    accessor.ReadArray<byte>(0, data, 0, data.Length);
                    Marshal.Copy(data, 0, p, (int)capacity);

                    //switch (Type)
                    //{
                    //    case 0:
                    //        targetData = (pinvoke.OBDATA)Marshal.PtrToStructure(p, typeof(pinvoke.OBDATA));
                    //        Console.WriteLine("OB " + ((pinvoke.OBDATA)targetData).PID);
                    //        break;
                    //    case 1:
                    //        targetData = (pinvoke.FSDATA)Marshal.PtrToStructure(p, typeof(pinvoke.FSDATA));
                    //        Console.WriteLine("FS " + ((pinvoke.FSDATA)targetData).FileName);
                    //        break;
                    //    case 2:
                    //        targetData = (pinvoke.REGDATA)Marshal.PtrToStructure(p, typeof(pinvoke.REGDATA));
                    //        Console.WriteLine("REG " + ((pinvoke.REGDATA)targetData).RegistryFullPath);
                    //        break;
                    //}

                    kernelEvent.Set();
                }
            }
            Marshal.FreeHGlobal(p);
        }
    }
}
