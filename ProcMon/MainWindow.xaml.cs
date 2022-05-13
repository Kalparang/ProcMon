using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ProcMon
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            Win32.AllocConsole();

            //int result;
            //result = DriverManage.CreateDriverServices();
            //if(result != 0)
            //{
            //    Console.WriteLine("CreateDriverServices : " + result);
            //    return;
            //}

            //for (int i = 0; i < 3; i++)
            //{
            //    new IPC().Init(i);

            //    result = DriverManage.StartService(i);
            //    if (result != 0)
            //    {
            //        Console.WriteLine("StartService : " + result);
            //        return;
            //    }
            //}

            //for (int i = 0; i < 3; i++)
            //{
            //    var bresult = DriverManage.StopService(i);
            //    if (bresult == false)
            //    {
            //        Console.WriteLine("StopService : " + bresult);
            //        return;
            //    }
            //}

            //result = DriverManage.DeleteServices();
            //if (result != 0)
            //{
            //    Console.WriteLine("CreateDriverServices : " + result);
            //    return;
            //}

            Console.WriteLine("end");
        }

        public class Win32
        {
            [DllImport("kernel32.dll")]
            public static extern Boolean AllocConsole();
            [DllImport("kernel32.dll")]
            public static extern Boolean FreeConsole();
        }
    }
}
