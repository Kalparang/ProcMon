using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Management;
using System.Text;
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
        Dictionary<long, List<string>> rejectList;
        public DataGrid uiGrid;
        public DataGrid processGrid;

        public MainViewModel()
        {
            DriverCollectionViewSource = new CollectionViewSource();
            DriverCollectionViewSource.Source = this.driverModels;
            DriverCollectionViewSource.Filter += ApplyFilter;

            ProcessCollectionViewSource = new CollectionViewSource();
            ProcessCollectionViewSource.Source = this.processModels;


            rejectList = new Dictionary<long, List<string>>();

            int result;

            result = DriverManage.CreateDriverServices();
            if (result != 0)
            {
                Console.WriteLine("CreateDriverServices : " + result);
                return;
            }

            Console.WriteLine("CreateService Success");

            IPC iPC = new IPC();
            iPC.rd += AddData;
            iPC.Init(pinvoke.DRIVER_TYPE.REGISTRY);

            DriverManage.StartService(pinvoke.DRIVER_TYPE.REGISTRY);

            //for (int i = 0; i < 3; i++)
            //{
            //    IPC iPC = new IPC();
            //    iPC.rd += AddData;
            //    iPC.Init(i);

            //    DriverManage.StartService(i);
            //}

            var process = Process.GetProcesses();
            foreach(var p in process)
            {
                var d = new Model.ProcessModel();
                d.PID = (UInt32)p.Id;
                d.ProcessName = p.ProcessName;
                d.fc += FilterChanged;
                processModels.Add(d);
            }

            var pw = new ProcessWatch();
            pw.re += ProcessEvent;
        }

        private void ApplyFilter(object sender, FilterEventArgs e)
        {
            Model.DriverModel item = (Model.DriverModel)e.Item;

            if (filterList.Count == 0)
            {
                e.Accepted = true;
            }
            else
            {
                e.Accepted = filterList.Contains(item.PID);
            }
        }

        List<int> _filterList;
        public List<int> filterList
        {
            get
            {
                if (_filterList == null)
                    _filterList = new List<int>();
                return _filterList;
            }
            set
            {
                _filterList = value;
            }
        }

        private void OnFilterChange()
        {
            DriverCollectionViewSource.View.Refresh();
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

        public void ProcessGrid_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            DataGrid dataGrid = (DataGrid)sender;

            if (dataGrid.SelectedItem == null)
                return;

            if(dataGrid.Name == uiGrid.Name)
            {
                Model.DriverModel model = (Model.DriverModel)dataGrid.SelectedItem;

                if(e.ChangedButton == MouseButton.Right)
                {
                    switch(model.Type)
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
            }
            else if(dataGrid.Name == processGrid.Name)
            {
                Model.ProcessModel p = (Model.ProcessModel)dataGrid.SelectedItem;

                if (e.ChangedButton == MouseButton.Right)
                {
                    pinvoke.BringProcessToFront(p.PID);
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

        public void FilterChanged(UInt32 PID, string ProcessName, bool IsFiltering)
        {
            if (IsFiltering)
            {
                if (!filterList.Contains((int)PID))
                {
                    filterList.Add((int)PID);
                    OnFilterChange();
                }
            }
            else
            {
                if(filterList.Contains((int)PID))
                {
                    filterList.Remove((int)PID);
                    OnFilterChange();
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
                    p.fc += FilterChanged;
                    Application.Current.Dispatcher.Invoke(
                        new Action(() => processModels.Add(p)));
                    break;
                case 1:
                    FilterChanged(p.PID, p.ProcessName, false);
                    Application.Current.Dispatcher.Invoke(
                        new Action(() => processModels.Remove(p)));
                    break;
            }
        }

        public void AddData(pinvoke.DRIVER_TYPE Type, object dataStruct)
        {
            Model.DriverModel driverModel = new Model.DriverModel();
            //bool UseData = true;

            switch(Type)
            {
                case pinvoke.DRIVER_TYPE.OB:
                    pinvoke.OBDATA ob = (pinvoke.OBDATA)dataStruct;
                    driverModel.date = new DateTime(ob.SystemTick);
                    driverModel.PID = (int)ob.PID;
                    driverModel.Act = ob.Operation.ToString();
                    driverModel.Target = ob.TargetPID.ToString();
                    //driverModel.TargetPID = (int)ob.TargetPID;
                    //driverModel.Operation = (int)ob.Operation;
                    //driverModel.DesiredAccess = (int)ob.DesiredAccess;
                    break;
                case pinvoke.DRIVER_TYPE.FILESYSTEM:
                    pinvoke.FSDATA fs = (pinvoke.FSDATA)dataStruct;

                    if (string.IsNullOrWhiteSpace(fs.FileName))
                        return;

                    if (fs.PID == 0 || fs.PID == 4)
                        return;

                    {
                        //string ProcessName = Process.GetProcessById((int)fs.PID).ProcessName;

                        //if(fs.PID == 4)
                        //{
                        //    try
                        //    {
                        //        //string targetName = fs.FileName.Split(',')[0];
                        //        //if (targetName.ToLower().StartsWith("system")
                        //        //    || targetName.StartsWith("dllhost")
                        //        //    || targetName.ToLower().StartsWith("registry")
                        //        //    || targetName.ToLower().StartsWith("textinputhost")
                        //        //    || targetName.ToLower().StartsWith("svchost")
                        //        //    || targetName.ToLower().StartsWith("microsoft")
                        //        //    || targetName.ToLower().StartsWith("MoUsoCoreWorke")
                        //        //    || targetName.ToLower().StartsWith("TrustedInstall")
                        //        //    || targetName.ToLower().StartsWith("TiWorker")
                        //        //    || targetName.ToLower().StartsWith("RuntimeBroker")
                        //        //    || targetName.ToLower().StartsWith("conhost")
                        //        //    || targetName.ToLower().StartsWith("WinSAT")
                        //        //    || targetName.ToLower().StartsWith("rundll32")
                        //        //    || targetName.ToLower().StartsWith("MoUsoCoreWorke")
                        //        //    )
                        //        //    return;
                        //        //foreach (Model.ProcessModel p in processModels)
                        //        //{
                        //        //    if (p.ProcessName.StartsWith(targetName))
                        //        //    {
                        //        //        if (!rejectList.ContainsKey(p.PID))
                        //        //            rejectList.Add(p.PID, new List<string>());
                        //        //        if (!rejectList[p.PID].Contains(fs.FileName))
                        //        //        {
                        //        //            rejectList[p.PID].Add(fs.FileName);
                        //        //        }

                        //        //        return;
                        //        //    }
                        //        //}
                        //    }
                        //    catch(Exception e)
                        //    {
                        //        Console.WriteLine(e.ToString());
                        //    }

                        //    return;
                        //}
                        //else if(fs.Flag == 8)
                        //{
                        //    if (!rejectList.ContainsKey(fs.PID))
                        //    {
                        //        rejectList.Add(fs.PID, new List<string>());
                        //        rejectList[fs.PID].Add("false");
                        //        rejectList[fs.PID].Add(ProcessName);
                        //    }
                        //    if (!rejectList[fs.PID].Contains(fs.FileName))
                        //    {
                        //        rejectList[fs.PID].Add(fs.FileName);
                        //    }
                        //    return;
                        //}
                        //else
                        //{
                        //    if (!rejectList.ContainsKey(fs.PID))
                        //    {
                        //        rejectList.Add(fs.PID, new List<string>());
                        //        rejectList[fs.PID].Add("false");
                        //        rejectList[fs.PID].Add(ProcessName);
                        //    }
                        //    if (rejectList[fs.PID][0] == "false")
                        //    {
                        //        if (!rejectList[fs.PID].Contains(fs.FileName))
                        //        {
                        //            rejectList[fs.PID].Add(fs.FileName);
                        //            var fnList = fs.FileName.Split('\\');
                        //            if (fnList[fnList.Length - 1] == rejectList[fs.PID][1])
                        //            {
                        //                if (ProcessName == "ProcMonTest")
                        //                    Console.WriteLine("Check - " + fs.FileName);

                        //                rejectList[fs.PID][0] = "true";
                        //            }
                        //        }
                        //        return;
                        //    }
                        //    if (rejectList.ContainsKey(fs.PID))
                        //    {
                        //        if (rejectList[fs.PID].Contains(fs.FileName))
                        //        {
                        //            if (ProcessName == "ProcMonTest")
                        //                Console.WriteLine(fs.FileName);

                        //            return;
                        //        }
                        //    }
                        //}
                    }

                    driverModel.date = new DateTime(fs.SystemTick);
                    driverModel.PID = (int)fs.PID;
                    driverModel.Act = fs.MajorFunction.ToString();
                    driverModel.Target = fs.FileName;
                    //driverModel.MajorFunction = (int)fs.MajorFunction;
                    //driverModel.FileName = fs.FileName;
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
                    //driverModel.NotifyClass = reg.NotifyClass;
                    //driverModel.RegistryFullPath = reg.RegistryFullPath;
                    break;
                default:
                    return;
            }

            driverModel.Type = Type;

            //if (UseData)
            {
                Application.Current.Dispatcher.Invoke(
                    new Action(() => driverModels.Add(driverModel))
                );
            }
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
}
