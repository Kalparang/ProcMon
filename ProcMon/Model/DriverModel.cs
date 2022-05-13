using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;

namespace ProcMon.Model
{
    public class DriverModel
    {
        public DateTime date { get; set; }
        public int PID { get; set; }

        public int? TargetPID { get; set; }
        public int? Operation { get; set; }
        public int? DesiredAccess { get; set; }

        public int? MajorFunction { get; set; }
        public string FileName { get; set; }

        public int? NotifyClass { get; set; }
        public string RegistryFullPath { get; set; }

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
        public UInt32 PID { get; set; }
        public string ProcessName { get; set; }

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
