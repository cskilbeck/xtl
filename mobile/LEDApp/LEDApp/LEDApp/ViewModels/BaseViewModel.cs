using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace LEDApp
{
    public class BaseViewModel : INotifyPropertyChanged
    {
        protected BaseViewModel()
        {
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            Log.Verbose($"{propertyName} changed!");
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
