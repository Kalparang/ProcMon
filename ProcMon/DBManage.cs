using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.SQLite;
using System.Diagnostics;
using System.Management;
using System.Threading;
using System.Windows;

namespace ProcMon
{
    public class DBManage
    {
        SQLiteConnection connection;
        Dictionary<int, List<WaitData>> WaitDatas;
        List<SQLiteCommand> insertList;
        List<Thread> threads;
        ManualResetEvent exitEvent;
        
        object lockObject;
        Dictionary<int, string> ProcessList;

        public DBManage(Dictionary<int, string> ProcessList)
        {
            this.ProcessList = ProcessList;
            exitEvent = new ManualResetEvent(false);
            string sqlFilePath = Path.Combine(Directory.GetCurrentDirectory(), "ProcMon.sqlite");
            string connectionString = $"Data Source={sqlFilePath}";
            WaitDatas = new Dictionary<int, List<WaitData>>();
            insertList = new List<SQLiteCommand>();
            lockObject = new object();

            connection = new SQLiteConnection(connectionString);
            connection.Open();

            CreateTable();
            threads = new List<Thread>();
            threads.Add(new Thread(new ThreadStart(InsertData)));
            threads.Add(new Thread(new ThreadStart(InsertThread)));

            foreach (Thread t in threads)
                t.Start();
        }

        public void Exit()
        {
            Console.WriteLine("~DBManage");
            exitEvent.Set();
            foreach (Thread t in threads)
                if (t.Join(1500) == false) t.Abort();
            connection.Close();
            connection.Dispose();
            Console.WriteLine("~DBManage Exit");
        }

        class dataQueue
        {
            public pinvoke.DRIVER_TYPE type;
            public object data;
            public string ProcessName = null;
            public string ProcessName2 = null;
        }

        List<dataQueue> dataList = new List<dataQueue>();

        public void insertQueue(pinvoke.DRIVER_TYPE type, object data, string ProcessName = null, string ProcessName2 = null)
        {
            dataList.Add(new dataQueue() { type = type, data = data, ProcessName = ProcessName, ProcessName2 = ProcessName2 });
        }

        void InsertThread()
        {
            while (true)
            {
                if (exitEvent.WaitOne(3000)) break;
                SQLiteCommand[] insertArrary;
                lock (lockObject)
                {
                    insertArrary = insertList.ToArray();
                    insertList.Clear();
                }

                using (SQLiteTransaction transaction = connection.BeginTransaction())
                {
                    foreach (var insert in insertArrary)
                    {
                        insert.ExecuteNonQuery();
                        insert.Dispose();
                    }
                    transaction.Commit();
                }
            }
            Console.WriteLine("InsertThread Thread Exit");
        }

        //public void InsertData(pinvoke.DRIVER_TYPE type, object data, string ProcessName = null, string ProcessName2 = null)
        public void InsertData()
        {
            while(true)
            {
                if (exitEvent.WaitOne(0)) break;

                if(dataList.Count == 0)
                {
                    System.Threading.Thread.Sleep(500);
                    continue;
                }

                dataQueue dq = dataList[0];
                if(dq == null)
                {
                    dataList.RemoveAt(0);
                    continue;
                }
                pinvoke.DRIVER_TYPE type = dq.type;
                object data = dq.data;
                string ProcessName = dq.ProcessName;
                string ProcessName2 = dq.ProcessName2;
                DateTime dataDate = DateTime.Now;

                dataList.RemoveAt(0);

                SQLiteCommand processinsertCommand = new SQLiteCommand();
                SQLiteCommand insertCommand = new SQLiteCommand();

                processinsertCommand.Connection = connection;
                processinsertCommand.CommandText = "insert or ignore into processes(process) values(@process)";
                processinsertCommand.Parameters.Add("@process", System.Data.DbType.String, 50);

                insertCommand.Connection = connection;
                insertCommand.CommandText = "insert into ";
                int WaitPID = 0;

                try
                {
                    switch (type)
                    {
                        case pinvoke.DRIVER_TYPE.OB:
                            pinvoke.OBDATA ob = (pinvoke.OBDATA)data;
                            insertCommand.CommandText += "targetprocesses(process, targetprocess, systemtick, desiredaccess) values ($process, $targetprocess, $systemtick, $desiredaccess)";
                            if (ProcessList.ContainsKey((int)ob.PID))
                                insertCommand.Parameters.AddWithValue("$process", ProcessList[(int)ob.PID]);
                            else
                            {
                                processinsertCommand.Dispose();
                                insertCommand.Dispose();
                                continue;
                            }
                            if(ProcessList.ContainsKey((int)ob.TargetPID))
                                insertCommand.Parameters.AddWithValue("$targetprocess", ProcessList[(int)ob.TargetPID]);
                            else
                            {
                                processinsertCommand.Dispose();
                                insertCommand.Dispose();
                                continue;
                            }
                            dataDate = new DateTime(ob.SystemTick);
                            insertCommand.Parameters.AddWithValue("$desiredaccess", ob.DesiredAccess);
                            break;
                        case pinvoke.DRIVER_TYPE.FILESYSTEM:
                            pinvoke.FSDATA fs = (pinvoke.FSDATA)data;
                            WaitPID = (int)fs.PID;
                            insertCommand.CommandText += "targetfiles(process, filename, systemtick, majorfunction) values ($process, $filename, $systemtick, $majorfunction)";
                            if (ProcessList.ContainsKey((int)fs.PID))
                                insertCommand.Parameters.AddWithValue("$process", ProcessList[(int)fs.PID]);
                            else
                            {
                                processinsertCommand.Dispose();
                                insertCommand.Dispose();
                                continue;
                            }
                            insertCommand.Parameters.AddWithValue("$filename", "C:" + fs.FileName);
                            dataDate = new DateTime(fs.SystemTick);
                            insertCommand.Parameters.AddWithValue("$majorfunction", fs.MajorFunction);
                            break;
                        case pinvoke.DRIVER_TYPE.REGISTRY:
                            pinvoke.REGDATA reg = (pinvoke.REGDATA)data;
                            WaitPID = (int)reg.PID;
                            insertCommand.CommandText += "targetregistries(process, registryfullpath, systemtick, notifyclass) values ($process, $registryfullpath, $systemtick, $notifyclass)";
                            if (ProcessList.ContainsKey((int)reg.PID))
                                insertCommand.Parameters.AddWithValue("$process", ProcessList[(int)reg.PID]);
                            else
                            {
                                processinsertCommand.Dispose();
                                insertCommand.Dispose();
                                continue;
                            }
                            insertCommand.Parameters.AddWithValue("$registryfullpath", reg.RegistryFullPath);
                            dataDate = new DateTime(reg.SystemTick);
                            insertCommand.Parameters.AddWithValue("$notifyclass", reg.NotifyClass);
                            break;
                        default:
                            continue;
                    }
                }
                catch (ArgumentException ae)
                {
                    processinsertCommand.Dispose();
                    insertCommand.Dispose();

                    //lock (lockObject)
                    //{
                    //    if (!WaitDatas.ContainsKey(WaitPID))
                    //        WaitDatas.Add(WaitPID, new List<WaitData>());
                    //    WaitDatas[WaitPID].Add(new WaitData(type, data));
                    //}
                    continue;
                }
                catch (InvalidOperationException ie)
                {
                    processinsertCommand.Dispose();
                    insertCommand.Dispose();

                    //lock (lockObject)
                    //{
                    //    if (!WaitDatas.ContainsKey(WaitPID))
                    //        WaitDatas.Add(WaitPID, new List<WaitData>());
                    //    WaitDatas[WaitPID].Add(new WaitData(type, data));
                    //}
                    continue;
                }

                dataDate = dataDate.ToLocalTime();
                dataDate = dataDate.AddYears(1600);
                insertCommand.Parameters.AddWithValue("$systemtick", dataDate.ToString("yyyy-MM-dd HH:mm:ss.fff"));
                processinsertCommand.Parameters[0].Value = insertCommand.Parameters[0].Value;

                lock (lockObject)
                {
                    insertList.Add(processinsertCommand);
                    insertList.Add(insertCommand);
                }
            }

            Console.WriteLine("InsertData Thread Exit");
        }

        public List<Model.ProcessModel> ReadProcesses()
        {
            List<Model.ProcessModel> result = new List<Model.ProcessModel>();
            string sql = "select * from processes";

            using(SQLiteCommand command = new SQLiteCommand(sql, connection))
            {
                SQLiteDataReader reader = command.ExecuteReader();

                while(reader.Read())
                {
                    string ProcessName = (string)reader["process"];
                    result.Add(new Model.ProcessModel() {
                        PID = 0,
                        ProcessName = ProcessName,
                    });
                }
            }

            return result;
        }

        public List<Model.DBModel> ReadItems(string ProcessName = "")
        {
            List<Model.DBModel> result = new List<Model.DBModel>();
            string obsql = "select * from targetprocesses";
            string fssql = "select * from targetfiles";
            string regsql = "select * from targetregistries";

            if(!string.IsNullOrWhiteSpace(ProcessName))
            {
                obsql += $" where process = '{ProcessName}'";
                fssql += $" where process = '{ProcessName}'";
                regsql += $" where process = '{ProcessName}'";
            }

            using (SQLiteCommand command = new SQLiteCommand(obsql, connection))
            {
                SQLiteDataReader reader = command.ExecuteReader();

                while (reader.Read())
                {
                    Model.DBModel item = new Model.DBModel();
                    item.date = (DateTime)reader["systemtick"];
                    item.Process = (string)reader["process"];
                    item.Act = string.Empty;
                    int DesiredAccess = Convert.ToInt32(reader["desiredaccess"]);
                    try
                    {
                        if ((DesiredAccess & (int)pinvoke.PROCESS_ACESS_MASK.PROCESS_ALL_ACCESS) == (int)pinvoke.PROCESS_ACESS_MASK.PROCESS_ALL_ACCESS)
                            item.Act = pinvoke.PROCESS_ACESS_MASK.PROCESS_ALL_ACCESS.ToString();
                        else
                            foreach (pinvoke.PROCESS_ACESS_MASK mask in Enum.GetValues(typeof(pinvoke.PROCESS_ACESS_MASK)))
                            {
                                if ((DesiredAccess & (int)mask) == (int)mask)
                                    item.Act += $"\n{mask.ToString()}";
                            }
                    }
                    catch (Exception e)
                    {

                    }
                    if (item.Act == string.Empty)
                        item.Act = DesiredAccess.ToString("X");
                    else
                        item.Act = item.Act.Trim('\n');

                    item.Target = (string)reader["targetprocess"];
                    item.Type = pinvoke.DRIVER_TYPE.OB;
                    result.Add(item);
                }
            }

            using (SQLiteCommand command = new SQLiteCommand(fssql, connection))
            {
                SQLiteDataReader reader = command.ExecuteReader();

                while (reader.Read())
                {
                    Model.DBModel item = new Model.DBModel();
                    item.date = (DateTime)reader["systemtick"];
                    item.Process = (string)reader["process"];
                    item.Act = ((pinvoke.IRP_MAJORFUNCTION)Convert.ToUInt32(reader["majorfunction"])).ToString();
                    item.Target = (string)reader["filename"];
                    item.Type = pinvoke.DRIVER_TYPE.FILESYSTEM;
                    result.Add(item);
                }
            }

            using (SQLiteCommand command = new SQLiteCommand(regsql, connection))
            {
                SQLiteDataReader reader = command.ExecuteReader();

                while (reader.Read())
                {
                    Model.DBModel item = new Model.DBModel();
                    item.date = (DateTime)reader["systemtick"];
                    item.Process = (string)reader["process"];
                    item.Act = ((pinvoke.REG_NOTIFY_CLASS)Convert.ToUInt32(reader["notifyclass"])).ToString();
                    string RegistryFullPath = (string)reader["registryfullpath"];
                    if (RegistryFullPath.Contains("\\\\"))
                        item.Target = RegistryFullPath.Split('\\')[1];
                    else
                        item.Target = RegistryFullPath;
                    item.Target = item.Target.Trim('\\');
                    item.Type = pinvoke.DRIVER_TYPE.REGISTRY;
                    result.Add(item);
                }
            }

            return result;
        }

        public void ProcessEvent(int Type, EventArrivedEventArgs e)
        {
            int PID = Convert.ToInt32(e.NewEvent.Properties["ProcessID"].Value);
            string ProcessName = (string)e.NewEvent.Properties["ProcessName"].Value;

            if (!WaitDatas.ContainsKey(PID))
                return;

            lock (lockObject)
            {
                while (WaitDatas[PID].Count > 0)
                {
                    string ProcessName1 = null;
                    string ProcessName2 = null;
                    WaitData data = WaitDatas[PID][0];
                    WaitDatas[PID].RemoveAt(0);

                    if (Type == 0)
                    {
                        if (((pinvoke.OBDATA)data.data).PID == PID) ProcessName1 = ProcessName;
                        else if (((pinvoke.OBDATA)data.data).TargetPID == PID) ProcessName2 = ProcessName;
                    }
                    else
                        ProcessName1 = ProcessName;

                    //InsertData(data.type, data.data, ProcessName1, ProcessName2);
                }

                WaitDatas.Remove(PID);
            }
        }

        public class WaitData
        {
            public pinvoke.DRIVER_TYPE type;
            public object data;

            public WaitData(pinvoke.DRIVER_TYPE type, object data)
            {
                this.type = type;
                this.data = data;
            }
        }

        void CreateTable()
        {
            string[] sqls =
            {
                "create table if not exists processes(" +
                "process varchar(50)," +
                "primary key(process)" +
                ") ",
                "create table if not exists targetprocesses(" +
                "process varchar(50)," +
                "targetprocess varchar(50)," +
                "systemtick datetime," +
                "desiredaccess int," +
                "foreign key(process) " +
                "references processes(process)" +
                ")",
                "create table if not exists targetfiles(" +
                "process varchar(50)," +
                "filename varchar(32767)," +
                "systemtick datetime," +
                "majorfunction int," +
                "foreign key(process) " +
                "references processes(process)" +
                ") ",
                "create table if not exists targetregistries(" +
                "process varchar(50)," +
                "registryfullpath varchar(32767)," +
                "systemtick datetime," +
                "notifyclass int," +
                "foreign key(process) " +
                "references processes(process)" +
                ") "
            };

            foreach (var sql in sqls)
            {
                using (SQLiteCommand command = new SQLiteCommand(sql, connection))
                {
                    command.ExecuteNonQuery();
                }
            }
        }
    }
}
