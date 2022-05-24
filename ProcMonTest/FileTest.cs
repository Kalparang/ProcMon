using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;

namespace ProcMonTest
{
    public class FileTest
    {
        List<FileStream> FileList;

        public FileTest()
        {
            FileList = new List<FileStream>();
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
            FileMode mode;
            FileAccess access;

            try
            {
                Console.WriteLine("\tOpenFile");
                Console.Write("\tPath\n> ");
                path = Console.ReadLine();

                Console.Write("\tFileMode\n\t1 : CreateNew\n\t2 : Create \n\t3 : Open\n\t4 : OpenOrCreate\n\t5 : Truncate\n\t6 : Append\n> ");
                input = Console.ReadLine();
                mode = (FileMode)int.Parse(input);

                Console.Write("\tFileAccess\n\t1 : Read\n\t2 : Write\n\t3: ReadWrite\n> ");
                input = Console.ReadLine();
                access = (FileAccess)int.Parse(input);

                var file = File.Open(path, mode, access);
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
                Console.Write("\tClose File Index : ");
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
                Console.Write("\tDelete File Path : ");
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

    }

    public class ProcessTest
    {
        List<Process> processes;

        public ProcessTest()
        {
            processes = new List<Process>();
        }

        public void InputCommand()
        {
            Console.WriteLine("1 : Start Process");
            Console.WriteLine("2 : Open Process");
            Console.WriteLine("3 : Print Process");
            Console.WriteLine("4 : Close Process");
            Console.WriteLine("5 : Terminate Process");
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
                        PrintProcess();
                        break;
                    case 4:
                        CloseProcess();
                        break;
                    case 5:
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
                processes.Add(Process.Start(path));
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
                processes.Add(Process.GetProcessById(int.Parse(input)));
            }
            catch(Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        public void PrintProcess()
        {
            for(int i = 0; i < processes.Count; i++)
            {
                Console.WriteLine(i + " : " + processes[i].Id + " | " + processes[i].ProcessName);
            }
        }

        public void CloseProcess()
        {
            Console.Write("CloseProcess Index\n> ");
            string input = Console.ReadLine();

            try
            {
                processes.RemoveAt(int.Parse(input));
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
