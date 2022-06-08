using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;
using Microsoft.Win32;

namespace ProcMonTest
{
    public class FileTest
    {
        List<FileStream> FileList;

        public FileTest()
        {
            FileList = new List<FileStream>();
        }

        public void InputCommand()
        {
            Console.WriteLine("1. PrintList\n2. OpenFile\n3.CloseFile\n4. DeleteFile");
            string input = Console.ReadLine();
            switch(input)
            {
                case "1":
                    PrintList();
                    break;
                case "2":
                    OpenFile();
                    break;
                case "3":
                    CloseFile();
                    break;
                case "4":
                    DeleteFile();
                    break;
            }
        }

        public void PrintList()
        {
            for (int i = 0; i < FileList.Count; i++)
            {
                Console.WriteLine("\t" + i + " : " + FileList[i].Name);
            }
        }

        public void OpenFile()
        {
            string input;
            string path;

            try
            {
                Console.Write("Path\n> ");
                path = Console.ReadLine();

                var file = File.Open(path, FileMode.OpenOrCreate);
                FileList.Add(file);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        public void CloseFile()
        {
            try
            {
                string input;
                int num;
                Console.Write("Close File Index : ");
                input = Console.ReadLine();
                num = int.Parse(input);

                FileList[num].Close();
                FileList.RemoveAt(num);
            }
            catch(Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        public void DeleteFile()
        {
            try
            {
                string input;
                Console.Write("Delete File Path : ");
                input = Console.ReadLine();
                File.Delete(input);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }
    }

    public class RegistryTest
    {
        public void InputCommand()
        {
            Console.WriteLine("1. CreateKey\n2. DeleteKey\n3. SetValue\n4. DeleteValue >");
            string input = Console.ReadLine();
            switch(input)
            {
                case "1":
                    CreateOrOpenKey();
                    break;
                case "2":
                    DeleteKey();
                    break;
                case "3":
                    SetKey();
                    break;
                case "4":
                    DeleteValue();
                    break;
            }
        }

        public void CreateOrOpenKey()
        {
            Console.WriteLine("Input RegistyrKey >");
            string input = Console.ReadLine();
            RegistryKey rk = Registry.CurrentUser.CreateSubKey(input);
            if(rk == null)
            {
                Console.WriteLine("Create or Open Key error");
                return;
            }
        }

        public void DeleteKey()
        {
            Console.WriteLine("Input RegistryKey > ");
            string input = Console.ReadLine();
            Registry.CurrentUser.DeleteSubKey(input);
        }
        public void DeleteValue()
        {
            Console.WriteLine("Input RegistryKey > ");
            string input = Console.ReadLine();
            RegistryKey rk = Registry.CurrentUser.CreateSubKey(input);
            if (rk == null)
            {
                Console.WriteLine("Create or Open Key error");
                return;
            }
            Console.WriteLine("Input Name > ");
            input = Console.ReadLine();
            rk.DeleteValue(input);
        }

        public void SetKey()
        {
            Console.WriteLine("Input RegistryKey > ");
            string input = Console.ReadLine();
            RegistryKey rk = Registry.CurrentUser.CreateSubKey(input);
            if (rk == null)
            {
                Console.WriteLine("Create or Open Key error");
                return;
            }
            Console.WriteLine("Input Name > ");
            input = Console.ReadLine();
            Console.WriteLine("Input Value > ");
            string input2 = Console.ReadLine();
            rk.SetValue(input, input2);
        }
    }

    public class ProcessTest
    {
        public void InputCommand()
        {
            Console.WriteLine("1 : Start Process");
            Console.WriteLine("2 : Open Process");
            Console.WriteLine("3 : Terminate Process");
            Console.Write("> ");
            string input = Console.ReadLine();

            try
            {
                int num = int.Parse(input);

                switch(num)
                {
                    case 1:
                        StartProcess();
                        break;
                    case 2:
                        OpenProcess();
                        break;
                    case 3:
                        TerminateProcess();
                        break;
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        public void StartProcess()
        {
            Console.Write("StartProcess Path\n> ");
            string path = Console.ReadLine();

            try
            {
                Process.Start(path);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        public void OpenProcess()
        {
            Console.WriteLine("OpenProcess PID\n> ");
            string input = Console.ReadLine();

            try
            {
                Process.GetProcessById(int.Parse(input));
            }
            catch(Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        public void TerminateProcess()
        {
            Console.Write("TerminateProcess PID\n> ");
            string input = Console.ReadLine();

            try
            {
                Process.GetProcessById(int.Parse(input)).Kill();
            }
            catch(Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }
    }
}
