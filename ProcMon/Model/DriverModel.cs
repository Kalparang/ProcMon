using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;

namespace ProcMon.Model
{
    public delegate void FilterChange(UInt32 PID, string ProcessName, bool IsFiltering);

    public class DriverModel
    {
        public DateTime date { get; set; }
        public int PID { get; set; }
        public string Act { get; set; }
        public string Target { get; set; }
        public pinvoke.DRIVER_TYPE Type { get; set; }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(obj, null)) return false;
            if (ReferenceEquals(this, obj)) return true;

            DriverModel other;

            try
            {
                other = (DriverModel)obj;
            }
            catch(InvalidCastException ie)
            {
                return false;
            }

            return this.Target == other.Target;
        }

            //public int? TargetPID { get; set; }
            //public int? Operation { get; set; }
            //public int? DesiredAccess { get; set; }

            //public int? MajorFunction { get; set; }
            //public string FileName { get; set; }

            //public int? NotifyClass { get; set; } 
            //public string RegistryFullPath { get; set; }

            //public event PropertyChangedEventHandler PropertyChanged;

            //protected void OnpropertyChanged(string propertyName)
            //{
            //    if(PropertyChanged != null)
            //    {
            //        PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            //    }
            //}
        }

    public class ProcessModel
    {
        public event FilterChange fc;

        public UInt32 PID { get; set; }
        public string ProcessName { get; set; }
        bool _IsFiltering { get; set; }

        public bool IsFiltering
        {
            get => _IsFiltering;
            set
            {
                _IsFiltering = value;
                if(fc != null)
                    fc(this.PID, this.ProcessName, this.IsFiltering);
            }
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(obj, null)) return false;
            if (ReferenceEquals(this, obj)) return true;

            ProcessModel other;

            try
            {
                other = (ProcessModel)obj;
            }
            catch(InvalidCastException ie)
            {
                return false;
            }

            return this.PID == other.PID;
        }
    }
}
