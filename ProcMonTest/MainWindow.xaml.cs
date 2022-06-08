using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.Threading;

namespace ProcMonTest
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

            new Thread(new ThreadStart(threadcontext)).Start();

            this.Close();
        }

        void threadcontext()
        {
            var f = new FileTest();
            var p = new ProcessTest();
            var r = new RegistryTest();

            while (true)
            {
                try
                {
                    Console.WriteLine("1.Process\n2.File\n3.Registry\n>");
                    string input = Console.ReadLine();
                    switch(input)
                    {
                        case "1":
                            p.InputCommand();
                            break;
                        case "2":
                            f.InputCommand();
                            break;
                        case "3":
                            r.InputCommand();
                            break;
                    }    
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.ToString());
                }
            }
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
