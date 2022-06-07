using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Management;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Threading;

namespace ProcMon.ViewModel
{
    public class MainViewModel : INotifyPropertyChanged
    {
        Dictionary<long, Dictionary<string, Model.DriverModel>> TargetList;
        DBManage db;
        List<Thread> threads;
        List<IPC> iPCs;
        ProcessWatch processWatch;
        ManualResetEvent exitEvent;
        pinvoke.DRIVER_TYPE FilterProcessType = pinvoke.DRIVER_TYPE.LAST;
        pinvoke.DRIVER_TYPE FilterDBType = pinvoke.DRIVER_TYPE.LAST;

        public CommandControl btn_cmd { get; set; }
        object ListLock;
        object ListLock2;

        Visibility _processlist_currentprocess;
        public Visibility processlist_currentprocess
        {
            get
            {
                return _processlist_currentprocess;
            }
            set
            {
                _processlist_currentprocess = value;
                OnPropertyChanged("processlist_currentprocess");
            }
        }
        Visibility _processlist_db;
        public Visibility processlist_db
        {
            get
            {
                return _processlist_db;
            }
            set
            {
                _processlist_db = value;
                if (processlist_db == Visibility.Visible)
                {
                    var processes = db.ReadProcesses();
                    foreach (var p in processes)
                    {
                        p.fc += FilterChanged;
                        processModels_DB.Add(p);
                    }
                    DriverCollectionViewSource_DB.View.Refresh();
                }
                else
                    processModels_DB.Clear();
                OnPropertyChanged("processlist_db");
            }
        }

        Visibility _itemlist_currenprocess;
        public Visibility itemlist_currentprocess
        {
            get
            {
                return _itemlist_currenprocess;
            }
            set
            {
                _itemlist_currenprocess = value;
                OnPropertyChanged("itemlist_currentprocess");
            }
        }
        Visibility _itemlist_db;
        public Visibility itemlist_db
        {
            get
            {
                return _itemlist_db;
            }
            set
            {
                _itemlist_db = value;
                if (itemlist_db == Visibility.Visible)
                {
                    //var items = db.ReadItems();
                    //foreach (var item in items)
                    //{
                    //    drivermodels_db.Add(item);
                    //}
                }
                else
                    drivermodels_db.Clear();

                OnPropertyChanged("itemlist_db");
            }
        }


        public MainViewModel()
        {
            Application.Current.Exit += Current_Exit;
            exitEvent = new ManualResetEvent(false);

            DriverCollectionViewSource = new CollectionViewSource();
            DriverCollectionViewSource.Source = this.driverModels;
            DriverCollectionViewSource.Filter += ApplyFilter;

            DriverCollectionViewSource_DB = new CollectionViewSource();
            DriverCollectionViewSource_DB.Source = this.drivermodels_db;
            DriverCollectionViewSource_DB.Filter += ApplyFilter;

            ProcessCollectionViewSource = new CollectionViewSource();
            ProcessCollectionViewSource.Source = this.processModels;

            ProcessCollectionViewSource_DB = new CollectionViewSource();
            ProcessCollectionViewSource_DB.Source = this._processModels_DB;

            btn_cmd = new CommandControl(Button_Event, CanExecute_Button);

            ListLock = new object();
            ListLock2 = new object();
            TargetList = new Dictionary<long, Dictionary<string, Model.DriverModel>>();

            db = new DBManage();

            processlist_currentprocess = Visibility.Visible;
            processlist_db = Visibility.Hidden;
            itemlist_currentprocess = Visibility.Visible;
            itemlist_db = Visibility.Hidden;

            int result;

            result = DriverManage.CreateDriverServices();
            if (result != 0)
            {
                Console.WriteLine("CreateDriverServices : " + result);
                return;
            }

            iPCs = new List<IPC>();
            for (pinvoke.DRIVER_TYPE i = 0; i < pinvoke.DRIVER_TYPE.LAST; i++)
            {
                IPC iPC = new IPC();
                iPC.rd += AddData;
                iPC.Init(i);
                iPCs.Add(iPC);

                DriverManage.StartService(i);
            }

            var process = Process.GetProcesses();
            foreach (var p in process)
            {
                var d = new Model.ProcessModel();
                d.PID = (UInt32)p.Id;
                d.ProcessName = p.ProcessName;
                d.fc += FilterChanged;
                processModels.Add(d);
            }

            processWatch = new ProcessWatch();
            processWatch.re += ProcessEvent;
            processWatch.re += db.ProcessEvent;

            threads = new List<Thread>();
            threads.Add(new Thread(new ThreadStart(AddDataThread)));
            //threads.Add(new Thread(new ThreadStart(RefreshThread)));
            foreach (Thread t in threads)
                t.Start();
        }

        void RefreshThread()
        {
            while (exitEvent.WaitOne(0) == false)
            {
                Application.Current.Dispatcher.Invoke(
                    new Action(() => DriverCollectionViewSource.View.Refresh())
                    );
                Thread.Sleep(1500);
            }
        }

        private void Current_Exit(object sender, ExitEventArgs e)
        {
            exitEvent.Set();
            db.Exit();
            for (pinvoke.DRIVER_TYPE i = 0; i < pinvoke.DRIVER_TYPE.LAST; i++)
                DriverManage.StopService(i);
            DriverManage.DeleteServices();
            foreach (IPC iPC in iPCs)
                iPC.Exit();
            processWatch.Stop();
            foreach (Thread t in threads)
                if (t.Join(1500) == false) t.Abort();
            Console.WriteLine("Current_Exit");
        }

        private void ApplyFilter(object sender, FilterEventArgs e)
        {
            CollectionViewSource viewSource = (CollectionViewSource)sender;

            Model.DriverModel currentprocess = e.Item as Model.DriverModel;
            Model.DBModel db = e.Item as Model.DBModel;

            if(currentprocess != null)
            {
                if(FilterProcessType != pinvoke.DRIVER_TYPE.LAST)
                {
                    if(currentprocess.Type != FilterProcessType)
                    {
                        e.Accepted = false;
                        return;
                    }
                }

                if (filterPIDList.Count > 0
                    && !string.IsNullOrWhiteSpace(filterString))
                {
                    e.Accepted = filterPIDList.Contains(currentprocess.PID)
                        && currentprocess.Target.ToLower().Contains(filterString.ToLower());
                }
                else if (filterPIDList.Count > 0)
                    e.Accepted = filterPIDList.Contains(currentprocess.PID);
                else if (!string.IsNullOrWhiteSpace(filterString))
                    e.Accepted = currentprocess.Target.ToLower().Contains(filterString.ToLower());
                else
                    e.Accepted = true;
            }
            if(db != null)
            {
                if (FilterDBType != pinvoke.DRIVER_TYPE.LAST)
                {
                    if (db.Type != FilterDBType)
                    {
                        e.Accepted = false;
                        return;
                    }
                }

                if (filterStringList.Count > 0
                    && !string.IsNullOrWhiteSpace(filterString))
                {
                    e.Accepted = filterStringList.Contains(db.Process.ToLower())
                        && db.Target.ToLower().Contains(filterString.ToLower());
                }
                else if (filterStringList.Count > 0)
                    e.Accepted = filterStringList.Contains(db.Process.ToLower());
                else if (!string.IsNullOrWhiteSpace(filterString))
                    e.Accepted = db.Target.ToLower().Contains(filterString.ToLower());
                else
                    e.Accepted = true;
            }
        }

        List<string> _filterstringList = null;
        public List<string> filterStringList
        {
            get
            {
                if (_filterstringList == null)
                    _filterstringList = new List<string>();
                return _filterstringList;
            }
            set
            {
                _filterstringList = value;
            }
        }
        List<int> _filterpidList;
        public List<int> filterPIDList
        {
            get
            {
                if (_filterpidList == null)
                    _filterpidList = new List<int>();
                return _filterpidList;
            }
            set
            {
                _filterpidList = value;
            }
        }

        string _filterString = null;
        public string filterString
        {
            get
            {
                if (_filterString == null)
                    _filterString = string.Empty;
                return _filterString;
            }
            set
            {
                _filterString = value;
                OnFilterChange();
            }
        }
        string _itemNumString = null;
        public string itemNumString
        {
            get
            {
                if (_itemNumString == null)
                    _itemNumString = string.Empty;
                return _itemNumString;
            }
            set
            {
                _itemNumString = value;
                OnPropertyChanged("itemNumString");
            }
        }
        string _itemNumString2 = null;
        public string itemNumString2
        {
            get
            {
                if (_itemNumString2 == null)
                    _itemNumString2 = string.Empty;
                return _itemNumString2;
            }
            set
            {
                _itemNumString2 = value;
                OnPropertyChanged("itemNumString2");
            }
        }


        string _itemAvgTime = "0";
        public string itemAvgTime
        {
            get
            {
                return _itemAvgTime;
            }
            set
            {
                _itemAvgTime = value;
                OnPropertyChanged("itemAvgTime");
            }
        }



        private void OnFilterChange()
        {
            if (itemlist_currentprocess == Visibility.Visible)
                DriverCollectionViewSource.View.Refresh();
            else
                DriverCollectionViewSource_DB.View.Refresh();
        }

        void Button_Event(object obj)
        {
            Button button = (Button)obj;

            switch(button.Name)
            {
                case "Button_List_Process":
                    processlist_currentprocess = Visibility.Visible;
                    processlist_db = Visibility.Hidden;
                    itemlist_currentprocess = Visibility.Visible;
                    itemlist_db = Visibility.Hidden;
                    break;
                case "Button_List_DB":
                    processlist_currentprocess = Visibility.Hidden;
                    processlist_db = Visibility.Visible;
                    itemlist_currentprocess = Visibility.Hidden;
                    itemlist_db = Visibility.Visible;
                    break;
                case "Button_Filter_All":
                    if (itemlist_currentprocess == Visibility.Visible)
                        FilterProcessType = pinvoke.DRIVER_TYPE.LAST;
                    else
                        FilterDBType = pinvoke.DRIVER_TYPE.LAST;
                    break;
                case "Button_Filter_OB":
                    if (itemlist_currentprocess == Visibility.Visible)
                        FilterProcessType = pinvoke.DRIVER_TYPE.OB;
                    else
                        FilterDBType = pinvoke.DRIVER_TYPE.OB;
                    break;
                case "Button_Filter_FS":
                    if (itemlist_currentprocess == Visibility.Visible)
                        FilterProcessType = pinvoke.DRIVER_TYPE.FILESYSTEM;
                    else
                        FilterDBType = pinvoke.DRIVER_TYPE.FILESYSTEM;
                    break;
                case "Button_Filter_REG":
                    if (itemlist_currentprocess == Visibility.Visible)
                        FilterProcessType = pinvoke.DRIVER_TYPE.REGISTRY;
                    else
                        FilterDBType = pinvoke.DRIVER_TYPE.REGISTRY;
                    break;
            }
            OnFilterChange();
        }

        bool CanExecute_Button(object obj)
        {
            return true;
        }

        CollectionViewSource ProcessCollectionViewSource { get; set; }
        public ICollectionView  ProcessCollection
        {
            get { return ProcessCollectionViewSource.View; }
        }
        ObservableCollection<Model.ProcessModel> _processModels = null;
        public ObservableCollection<Model.ProcessModel> processModels
        {
            get
            {
                if(_processModels == null)
                {
                    _processModels = new ObservableCollection<Model.ProcessModel>();
                }
                return _processModels;
            }
            set
            {
                _processModels = value;
            }
        }

        CollectionViewSource ProcessCollectionViewSource_DB { get; set; }
        public ICollectionView ProcessCollection_DB
        {
            get { return ProcessCollectionViewSource_DB.View; }
        }
        ObservableCollection<Model.ProcessModel> _processModels_DB = null;
        public ObservableCollection<Model.ProcessModel> processModels_DB
        {
            get
            {
                if (_processModels_DB == null)
                {
                    _processModels_DB = new ObservableCollection<Model.ProcessModel>();
                }
                return _processModels_DB;
            }
            set
            {
                _processModels_DB = value;
            }
        }

        public void Grid_MouseRightClick(object sender, MouseEventArgs e)
        {
            DataGrid dataGrid = (DataGrid)sender;

            if (dataGrid.SelectedItem == null)
                return;

            if(dataGrid.Name == "uiGrid")
            {
                Model.DriverModel model = (Model.DriverModel)dataGrid.SelectedItem;

                switch (model.Type)
                {
                    case pinvoke.DRIVER_TYPE.OB:
                        Clipboard.SetText(model.Target);
                        break;
                    case pinvoke.DRIVER_TYPE.FILESYSTEM:
                        try
                        {
                            Clipboard.SetText(model.Target);
                            string Path = model.Target;
                            Process.Start("explorer.exe", "/select, " + Path);
                        }
                        catch (Exception ex)
                        {

                        }
                        break;
                    case pinvoke.DRIVER_TYPE.REGISTRY:
                        Clipboard.SetText(model.Target);
                        break;
                }
            }
            else if(dataGrid.Name == "itemGrid_DB")
            {
                Model.DBModel model = (Model.DBModel)dataGrid.SelectedItem;

                switch (model.Type)
                {
                    case pinvoke.DRIVER_TYPE.OB:
                        Clipboard.SetText(model.Target);
                        break;
                    case pinvoke.DRIVER_TYPE.FILESYSTEM:
                        try
                        {
                            string Path = "C:" + model.Target;
                            Process.Start("explorer.exe", "/select, " + Path);
                        }
                        catch (Exception ex)
                        {

                        }
                        break;
                    case pinvoke.DRIVER_TYPE.REGISTRY:
                        Clipboard.SetText(model.Target);
                        break;
                }
            }
            else if(dataGrid.Name == "ProcessGrid")
            {
                Model.ProcessModel p = (Model.ProcessModel)dataGrid.SelectedItem;

                pinvoke.BringProcessToFront(p.PID);
            }
            else if(dataGrid.Name == "ProcessGrid_DB")
            {
                drivermodels_db.Clear();
                Model.ProcessModel p = (Model.ProcessModel)dataGrid.SelectedItem;

                var items = db.ReadItems(p.ProcessName);
                foreach (var item in items)
                {
                    drivermodels_db.Add(item);
                }
            }
        }

        CollectionViewSource DriverCollectionViewSource { get; set; }
        public ICollectionView DriverCollection
        {
            get { return DriverCollectionViewSource.View; }
        }
        ObservableCollection<Model.DriverModel> _driverModels = null;
        public ObservableCollection<Model.DriverModel> driverModels
        {
            get
            {
                if(_driverModels == null)
                {
                    _driverModels = new ObservableCollection<Model.DriverModel>();
                }
                return _driverModels;
            }
            set
            {
                _driverModels = value;
            }
        }

        CollectionViewSource DriverCollectionViewSource_DB { get; set; }
        public ICollectionView DriverCollection_DB
        {
            get { return DriverCollectionViewSource_DB.View; }
        }
        ObservableCollection<Model.DBModel> _drivermodels_db = null;
        public ObservableCollection<Model.DBModel> drivermodels_db
        {
            get
            {
                if (_drivermodels_db == null)
                {
                    _drivermodels_db = new ObservableCollection<Model.DBModel>();
                }
                return _drivermodels_db;
            }
            set
            {
                _drivermodels_db = value;
            }
        }

        public void FilterChanged(UInt32 PID, string ProcessName, bool IsFiltering)
        {
            if (processlist_currentprocess == Visibility.Visible)
            {
                if (IsFiltering)
                {
                    if (!filterPIDList.Contains((int)PID))
                    {
                        filterPIDList.Add((int)PID);
                        OnFilterChange();
                    }
                }
                else
                {
                    if (filterPIDList.Contains((int)PID))
                    {
                        filterPIDList.Remove((int)PID);
                        OnFilterChange();
                    }
                }
            }
            else
            {
                if(IsFiltering)
                {
                    if(!filterStringList.Contains(ProcessName))
                    {
                        filterStringList.Add(ProcessName);
                        OnFilterChange();
                    }
                }
                else
                {
                    if (filterStringList.Contains(ProcessName))
                    {
                        filterStringList.Remove(ProcessName);
                        OnFilterChange();
                    }
                }
            }
        }

        public void ProcessEvent(int Type, EventArrivedEventArgs e)
        {
            var p = new Model.ProcessModel();

            p.PID = (UInt32)e.NewEvent.Properties["ProcessID"].Value;
            p.ProcessName = (string)e.NewEvent.Properties["ProcessName"].Value;

            switch (Type)
            {
                case 0:
                    if(processModels.Contains(p))
                    {
                        FilterChanged(p.PID, p.ProcessName, false);
                        lock (ListLock)
                            TargetList.Remove(p.PID);
                        Application.Current.Dispatcher.Invoke(
                            new Action(() => processModels.Remove(p)));
                    }

                    p.fc += FilterChanged;
                    lock (ListLock)
                    {
                        if (!TargetList.ContainsKey(p.PID))
                            TargetList.Add(p.PID, new Dictionary<string, Model.DriverModel>());
                    }
                    Application.Current.Dispatcher.Invoke(
                        new Action(() => processModels.Add(p)));
                    break;
                //case 1:
                //    FilterChanged(p.PID, p.ProcessName, false);
                //    lock(ListLock)
                //        TargetList.Remove(p.PID);
                //    Application.Current.Dispatcher.Invoke(
                //        new Action(() => processModels.Remove(p)));
                //    break;
            }
        }

        class InputDataClass
        {
            public pinvoke.DRIVER_TYPE Type;
            public object dataStruct;
        }

        List<InputDataClass> InputDataList = new List<InputDataClass>();

        public void AddData(pinvoke.DRIVER_TYPE Type, object dataStruct)
        {
            lock (ListLock2)
            {
                InputDataList.Add(new InputDataClass() { Type = Type, dataStruct = dataStruct });
                itemNumString2 = InputDataList.Count.ToString();
            }
            //new System.Threading.Thread(() => AddDataThread(Type, dataStruct)).Start();
        }

        //public void AddDataThread(pinvoke.DRIVER_TYPE Type, object dataStruct)
        public void AddDataThread()
        {
            while (true)
            {
                if (exitEvent.WaitOne(0))
                    break;
                if (InputDataList.Count == 0)
                {
                    System.Threading.Thread.Sleep(500);
                    continue;
                }

                InputDataClass data = null;
                pinvoke.DRIVER_TYPE Type = pinvoke.DRIVER_TYPE.OB;
                object dataStruct = null;

                lock (ListLock2)
                {
                    try
                    {
                        data = InputDataList[0];
                        if (data == null)
                        {
                            InputDataList.RemoveAt(0);
                            continue;
                        }
                        Type = data.Type;
                        dataStruct = data.dataStruct;
                        InputDataList.RemoveAt(0);
                        itemNumString2 = InputDataList.Count.ToString();
                    }
                    catch (Exception e)
                    {
                        continue;
                    }
                }

                Model.DriverModel driverModel = new Model.DriverModel();

                switch (Type)
                {
                    case pinvoke.DRIVER_TYPE.OB:
                        pinvoke.OBDATA ob = (pinvoke.OBDATA)dataStruct;

                        driverModel.date = new DateTime(ob.SystemTick);
                        driverModel.PID = (int)ob.PID;
                        driverModel.Act = string.Empty;
                        try
                        {
                            if (((int)ob.DesiredAccess & (int)pinvoke.PROCESS_ACESS_MASK.PROCESS_ALL_ACCESS) == (int)pinvoke.PROCESS_ACESS_MASK.PROCESS_ALL_ACCESS)
                                driverModel.Act = pinvoke.PROCESS_ACESS_MASK.PROCESS_ALL_ACCESS.ToString();
                            else
                                foreach (pinvoke.PROCESS_ACESS_MASK mask in Enum.GetValues(typeof(pinvoke.PROCESS_ACESS_MASK)))
                                {
                                    if (((int)ob.DesiredAccess & (int)mask) == (int)mask)
                                        driverModel.Act += $"\n{mask.ToString()}";
                                }
                        }
                        catch (Exception e)
                        {

                        }

                        if (driverModel.Act == string.Empty)
                            driverModel.Act = ob.DesiredAccess.ToString("X");
                        else
                            driverModel.Act = driverModel.Act.Trim('\n');

                        driverModel.Target = ob.TargetPID.ToString();
                        try
                        {
                            driverModel.Target += " | " + Process.GetProcessById((int)ob.TargetPID).ProcessName;
                        }
                        catch (Exception e) { }
                        //driverModel.TargetPID = (int)ob.TargetPID;
                        //driverModel.Operation = (int)ob.Operation;
                        //driverModel.DesiredAccess = (int)ob.DesiredAccess;
                        break;
                    case pinvoke.DRIVER_TYPE.FILESYSTEM:
                        pinvoke.FSDATA fs = (pinvoke.FSDATA)dataStruct;

                        if (string.IsNullOrWhiteSpace(fs.FileName))
                            continue;

                        if (fs.PID == 0 || fs.PID == 4)
                            continue;

                        driverModel.date = new DateTime(fs.SystemTick);
                        driverModel.PID = (int)fs.PID;
                        driverModel.Act = fs.MajorFunction.ToString();
                        if(fs.DeletePending == 1 || fs.DeleteAccess == 1)
                            driverModel.Act += "\nDelete";
                        driverModel.Target = "C:" + fs.FileName;
                        break;
                    case pinvoke.DRIVER_TYPE.REGISTRY:
                        pinvoke.REGDATA reg = (pinvoke.REGDATA)dataStruct;
                        driverModel.date = new DateTime(reg.SystemTick);
                        driverModel.PID = (int)reg.PID;
                        driverModel.Act = reg.NotifyClass.ToString();
                        if (reg.RegistryFullPath.Contains("\\\\"))
                            driverModel.Target = reg.RegistryFullPath.Split('\\')[1];
                        else
                            driverModel.Target = reg.RegistryFullPath;
                        driverModel.Target = driverModel.Target.Trim('\\');
                        //driverModel.NotifyClass = reg.NotifyClass;
                        //driverModel.RegistryFullPath = reg.RegistryFullPath;
                        break;
                    default:
                        continue;
                }
                driverModel.date = driverModel.date.ToLocalTime();
                driverModel.date = driverModel.date.AddYears(1600);
                driverModel.Type = Type;

                db.insertQueue(Type, dataStruct);

                //Application.Current.Dispatcher.Invoke(
                //    new Action(() => driverModels.Add(driverModel))
                //    );
                //itemNumString = driverModels.Count.ToString();

                lock (ListLock)
                {
                    if (!TargetList.ContainsKey(driverModel.PID))
                    {
                        TargetList.Add(driverModel.PID, new Dictionary<string, Model.DriverModel>());
                    }

                    string dickKey = driverModel.Target.ToLower() + driverModel.Act.ToLower();

                    if (!TargetList[driverModel.PID].ContainsKey(dickKey))
                    {
                        TargetList[driverModel.PID].Add(dickKey, driverModel);
                        Application.Current.Dispatcher.Invoke(
                            new Action(() => driverModels.Add(driverModel))
                            );
                        itemNumString = driverModels.Count.ToString();
                    }
                    else
                    {
                        Model.DriverModel target = TargetList[driverModel.PID][dickKey];
                        if (driverModel.date > target.date)
                        {
                            target.date = driverModel.date;
                            //Application.Current.Dispatcher.BeginInvoke(
                            //    new Action(() => DriverCollectionViewSource.View.Refresh())
                            //    );
                        }
                    }
                }

                TimeSpan ts = DateTime.Now - driverModel.date;
                itemAvgTime = Math.Round((ts.TotalSeconds + double.Parse(itemAvgTime)) / 2, 3).ToString();
            }
            Console.WriteLine("AddDataThread Thread Exit");
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if(handler != null)
            {
                handler(this, new PropertyChangedEventArgs(name));
            }
        }
    }

    public class CommandControl : ICommand
    {
        Action<object> ExecuteMethod;
        Func<object, bool> CanExecuteMethod;
        public event EventHandler CanExecuteChanged;

        public CommandControl(Action<object> execute_method, Func<object, bool> canexecute_method)
        {
            this.ExecuteMethod = execute_method;
            this.CanExecuteMethod = canexecute_method;
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            ExecuteMethod(parameter);
        }
    }
}
