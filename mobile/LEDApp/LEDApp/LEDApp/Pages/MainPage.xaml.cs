using System.Timers;
using Xamarin.Forms;

namespace LEDApp
{
    public partial class MainPage : ContentPage
    {
        public static int max_strings => 6;

        public static UDP[] udp;
        public string text { get; set; }

        private Animation _animation;
        private double _initialHeight;

        private Timer _status_timer = new Timer(1000.0);

        public MainPage()
        {
            InitializeComponent();

            MainViewModel model = new MainViewModel(AnimateRow) {
                ColorLow = Color.FromRgb(0, 0, 0),
                ColorHigh = Color.FromRgb(1, 1, 1)
            };
            BindingContext = model;

            if(ListViewRow != null)
            {
                _initialHeight = ListViewRow.Height.Value;
            }
            else
            {
                _initialHeight = 200;
            }
            udp = new UDP[max_strings];
            for (int i = 0; i < max_strings; ++i)
            {
                udp[i] = new UDP(model);
                udp[i].start_sending("192.168.0.30", 9173 + i);
            }

            //_status_timer.Elapsed += _status_timer_Elapsed;
            //_status_timer.AutoReset = true;
            //_status_timer.Start();
        }

        private void _status_timer_Elapsed(object sender, ElapsedEventArgs e)
        {
            //Log.Debug($"Sending one");
            //udp[0].update();
        }

        private void AnimateRow()
        {
            if (ListViewRow.Height.Value < _initialHeight)
            {
                _animation = new Animation(
                    (d) =>
                    {
                        ListViewRow.Height = new GridLength(Clamp(d, 0, double.MaxValue));
                    },

                    ListViewRow.Height.Value,
                    _initialHeight,
                    
                    Easing.CubicInOut,
                    () => _animation = null);
            }
            else
            {
                _animation = new Animation(
                    (d) =>
                    {
                        ListViewRow.Height = new GridLength(Clamp(d, 0, double.MaxValue));
                    },
                    _initialHeight,
                    0,
                    
                    Easing.CubicInOut,
                    () => _animation = null);

            }
            _animation.Commit(this, "the animation");
        }

        // Make sure we don't go below zero
        private double Clamp(double value, double minValue, double maxValue)
        {
            if (value < minValue)
            {
                return minValue;
            }

            if (value > maxValue)
            {
                return maxValue;
            }

            return value;
        }
    }
}
