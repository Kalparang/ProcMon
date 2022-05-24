using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.SQLite;
using System.Diagnostics;
using System.Management;

namespace ProcMon
{
    public class DBManage
    {
        SQLiteConnection connection;
        Dictionary<int, List<WaitData>> WaitDatas;
        List<SQLiteCommand> insertList;
        object lockObject;

        public DBManage()
        {
            string currentPath = Path.GetDirectoryName(Environment.CurrentDirectory);
            if (currentPath == null) currentPath = @"C:\proc";
            string sqlFilePath = Path.Combine(currentPath, "ProcMon.sqlite");
            string connectionString = $"Data Source={sqlFilePath}";
            WaitDatas = new Dictionary<int, List<WaitData>>();
            insertList = new List<SQLiteCommand>();
            lockObject = new object();

            connection = new SQLiteConnection(connectionString);
            connection.Open();

            CreateTable();
            new System.Threading.Thread(new System.Threading.ThreadStart(InsertThread)).Start();
        }

        void InsertThread()
        {
            while (true)
            {
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

                System.Threading.Thread.Sleep(1000);
            }
        }

        public void InsertData(pinvoke.DRIVER_TYPE type, object data, string ProcessName = null, string ProcessName2 = null)
        {
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
                        WaitPID = (int)ob.PID;
                        if (ProcessName != null)
                            insertCommand.Parameters.AddWithValue("$process", ProcessName);
                        else
                            insertCommand.Parameters.AddWithValue("$process", Process.GetProcessById((int)ob.PID).ProcessName);
                        WaitPID = (int)ob.TargetPID;
                        if (ProcessName2 != null)
                            insertCommand.Parameters.AddWithValue("$targetprocess", ProcessName2);
                        else
                            insertCommand.Parameters.AddWithValue("$targetprocess", Process.GetProcessById((int)ob.TargetPID).ProcessName);
                        insertCommand.Parameters.AddWithValue("$systemtick", new DateTime(ob.SystemTick).ToString("yyyy-MM-dd HH:mm:ss.fff"));
                        insertCommand.Parameters.AddWithValue("$desiredaccess", ob.DesiredAccess);

                        //insertCommand.Parameters.Add("$process", System.Data.DbType.String, 50);
                        //insertCommand.Parameters.Add("$targetprocess", System.Data.DbType.String, 50);
                        //insertCommand.Parameters.Add("$systemtick", System.Data.DbType.DateTime);
                        //insertCommand.Parameters.Add("$desiredaccess", System.Data.DbType.UInt32);
                        //WaitPID = (int)ob.PID;
                        //insertCommand.Parameters[0].Value = Process.GetProcessById((int)ob.PID).ProcessName;
                        //WaitPID = (int)ob.TargetPID;
                        //insertCommand.Parameters[1].Value = Process.GetProcessById((int)ob.TargetPID).ProcessName;
                        //insertCommand.Parameters[2].Value = new DateTime(ob.SystemTick).ToString("yyyy-MM-dd HH:mm:ss.fff");
                        //insertCommand.Parameters[3].Value = ob.DesiredAccess;
                        break;
                    case pinvoke.DRIVER_TYPE.FILESYSTEM:
                        pinvoke.FSDATA fs = (pinvoke.FSDATA)data;
                        WaitPID = (int)fs.PID;
                        insertCommand.CommandText += "targetfiles(process, filename, systemtick, majorfunction) values ($process, $filename, $systemtick, $majorfunction)";
                        if (ProcessName != null)
                            insertCommand.Parameters.AddWithValue("$process", ProcessName);
                        else
                            insertCommand.Parameters.AddWithValue("$process", Process.GetProcessById((int)fs.PID).ProcessName);
                        insertCommand.Parameters.AddWithValue("$filename", fs.FileName);
                        insertCommand.Parameters.AddWithValue("$systemtick", new DateTime(fs.SystemTick).ToString("yyyy-MM-dd HH:mm:ss.fff"));
                        insertCommand.Parameters.AddWithValue("$majorfunction", fs.MajorFunction);

                        //insertCommand.Parameters.Add("$process", System.Data.DbType.String, 50);
                        //insertCommand.Parameters.Add("$filename", System.Data.DbType.String, 32767);
                        //insertCommand.Parameters.Add("$systemtick", System.Data.DbType.DateTime);
                        //insertCommand.Parameters.Add("$majorfunciton", System.Data.DbType.UInt32);
                        //insertCommand.Parameters[0].Value = Process.GetProcessById((int)fs.PID).ProcessName;
                        //insertCommand.Parameters[1].Value = fs.FileName;
                        //insertCommand.Parameters[2].Value = new DateTime(fs.SystemTick).ToString("yyyy-MM-dd HH:mm:ss.fff");
                        //insertCommand.Parameters[3].Value = fs.MajorFunction;
                        break;
                    case pinvoke.DRIVER_TYPE.REGISTRY:
                        pinvoke.REGDATA reg = (pinvoke.REGDATA)data;
                        WaitPID = (int)reg.PID;
                        insertCommand.CommandText += "targetregistries(process, registryfullpath, systemtick, notifyclass) values ($process, $registryfullpath, $systemtick, $notifyclass)";
                        if (ProcessName != null)
                            insertCommand.Parameters.AddWithValue("$process", ProcessName);
                        else
                            insertCommand.Parameters.AddWithValue("$process", Process.GetProcessById((int)reg.PID).ProcessName);
                        insertCommand.Parameters.AddWithValue("$registryfullpath", reg.RegistryFullPath);
                        insertCommand.Parameters.AddWithValue("$systemtick", new DateTime(reg.SystemTick).ToString("yyyy-MM-dd HH:mm:ss.fff"));
                        insertCommand.Parameters.AddWithValue("$notifyclass", reg.NotifyClass);

                        //insertCommand.Parameters.Add("$process", System.Data.DbType.String, 50);
                        //insertCommand.Parameters.Add("$registryfullpath", System.Data.DbType.String, 32767);
                        //insertCommand.Parameters.Add("$systemtick", System.Data.DbType.DateTime);
                        //insertCommand.Parameters.Add("$notifyclass", System.Data.DbType.UInt32);
                        //insertCommand.Parameters[0].Value = Process.GetProcessById((int)reg.PID).ProcessName;
                        //insertCommand.Parameters[1].Value = reg.RegistryFullPath;
                        //insertCommand.Parameters[2].Value = new DateTime(reg.SystemTick).ToString("yyyy-MM-dd HH:mm:ss.fff");
                        //insertCommand.Parameters[3].Value = reg.NotifyClass;
                        break;
                    default:
                        return;
                }
            }
            catch (ArgumentException ae)
            {
                processinsertCommand.Dispose();
                insertCommand.Dispose();

                lock (lockObject)
                {
                    if (!WaitDatas.ContainsKey(WaitPID))
                        WaitDatas.Add(WaitPID, new List<WaitData>());
                    WaitDatas[WaitPID].Add(new WaitData(type, data));
                }
                return;
            }
            catch (InvalidOperationException ie)
            {
                processinsertCommand.Dispose();
                insertCommand.Dispose();

                lock (lockObject)
                {
                    if (!WaitDatas.ContainsKey(WaitPID))
                        WaitDatas.Add(WaitPID, new List<WaitData>());
                    WaitDatas[WaitPID].Add(new WaitData(type, data));
                }
                return;
            }

            processinsertCommand.Parameters[0].Value = insertCommand.Parameters[0].Value;

            lock (lockObject)
            {
                insertList.Add(processinsertCommand);
                insertList.Add(insertCommand);
            }
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

        public List<Model.DBModel> ReadItems()
        {
            List<Model.DBModel> result = new List<Model.DBModel>();
            string obsql = "select * from targetprocesses";
            string fssql = "select * from targetfiles";
            string regsql = "select * from targetregistries";

            using (SQLiteCommand command = new SQLiteCommand(obsql, connection))
            {
                SQLiteDataReader reader = command.ExecuteReader();

                while (reader.Read())
                {
                    Model.DBModel item = new Model.DBModel();
                    item.date = (DateTime)reader["systemtick"];
                    item.Process = (string)reader["process"];
                    item.Act = ((pinvoke.ACCESS_MASK)Convert.ToUInt32(reader["desiredaccess"])).ToString();
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
                    item.Target = (string)reader["registryfullpath"];
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

                    InsertData(data.type, data.data, ProcessName1, ProcessName2);
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
