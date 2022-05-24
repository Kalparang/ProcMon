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
        }

        void threadcontext()
        {
            var f = new FileTest();
            var p = new ProcessTest();

            while (true)
            {
                try
                {
                    p.InputCommand();
                    //string input;
                    //int num;
                    //Console.Write("File\n1 : Print List\n2 : Open File\n 3 : Close File\n4 : Delete File\n> ");
                    //input = Console.ReadLine();
                    //num = int.Parse(input);

                    //switch (num)
                    //{
                    //    case 1:
                    //        f.PrintList();
                    //        break;
                    //    case 2:
                    //        f.OpenFile();
                    //        break;
                    //    case 3:
                    //        f.CloseFile();
                    //        break;
                    //    case 4:
                    //        f.DeleteFile();
                    //        break;
                    //}
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
