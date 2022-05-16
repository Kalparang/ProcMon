using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

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
}
