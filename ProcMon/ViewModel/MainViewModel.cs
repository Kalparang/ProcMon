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
using System.Windows.Threading;

namespace ProcMon.ViewModel
{
    class MainViewModel : INotifyPropertyChanged
    {
        public MainViewModel()
        {
            //int result;

            //result = DriverManage.CreateDriverServices();
            //if (result != 0)
            //{
            //    Console.WriteLine("CreateDriverServices : " + result);
            //    return;
            //}

            //Console.WriteLine("CreateService Success");

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
                processModels.Add(d);
            }

            var pw = new ProcessWatch();
            pw.re += ProcessEvent;
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

        public void ProcessEvent(int Type, EventArrivedEventArgs e)
        {
            var p = new Model.ProcessModel();

            p.PID = (UInt32)e.NewEvent.Properties["ProcessID"].Value;
            p.ProcessName = (string)e.NewEvent.Properties["ProcessName"].Value;

            switch (Type)
            {
                case 0:
                    Application.Current.Dispatcher.Invoke(
                        new Action(() => processModels.Add(p)));
                    break;
                case 1:
                    Application.Current.Dispatcher.Invoke(
                        new Action(() => processModels.Remove(p)));
                    break;
            }
        }

        public void AddData(int Type, object dataStruct)
        {
            Model.DriverModel driverModel = new Model.DriverModel();

            switch(Type)
            {
                case 0:
                    pinvoke.OBDATA ob = (pinvoke.OBDATA)dataStruct;
                    driverModel.date = new DateTime(ob.SystemTick);
                    driverModel.PID = (int)ob.PID;
                    driverModel.TargetPID = (int)ob.TargetPID;
                    driverModel.Operation = (int)ob.Operation;
                    driverModel.DesiredAccess = (int)ob.DesiredAccess;
                    break;
                case 1:
                    pinvoke.FSDATA fs = (pinvoke.FSDATA)dataStruct;
                    driverModel.date = new DateTime(fs.SystemTick);
                    driverModel.PID = (int)fs.PID;
                    driverModel.MajorFunction = (int)fs.MajorFunction;
                    driverModel.FileName = fs.FileName;
                    break;
                case 2:
                    pinvoke.REGDATA reg = (pinvoke.REGDATA)dataStruct;
                    driverModel.date = new DateTime(reg.SystemTick);
                    driverModel.PID = (int)reg.PID;
                    driverModel.NotifyClass = reg.NotifyClass;
                    driverModel.RegistryFullPath = reg.RegistryFullPath;
                    break;
                default:
                    return;
            }

            Application.Current.Dispatcher.Invoke(
                new Action(() => driverModels.Add(driverModel))
            );
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
