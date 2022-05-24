using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace ProcMon
{
    public class ButtonCommand : ICommand
    {
        Action<object> ExecuteMethod;
        Func<object, bool> CanExecuteMethod;
        public event EventHandler CanExecuteChanged;

        public ButtonCommand(Action<object> execute_method, Func<object, bool> canexecute_method)
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
