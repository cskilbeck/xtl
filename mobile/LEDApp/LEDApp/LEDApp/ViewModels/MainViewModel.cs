using System;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Xamarin.Forms;

namespace LEDApp
{
    public class MainViewModel : BaseViewModel
    {
        double redLow_ = 0;
        double redHigh_ = 1;

        double greenLow_ = 0;
        double greenHigh_ = 1;

        double blueLow_ = 0;
        double blueHigh_ = 1;

        double speedLow_ = 0;
        double speedHigh_ = 1;

        Color colorLow_;
        Color colorHigh_;

        string debugString_ = "DEBUG";

        EffectViewModel currentEffect_ = null;
        ObservableCollection<EffectViewModel> effects_;

        private Action _rowAnimation;

        public bool[] stringEnabled = new bool[MainPage.max_strings];

        public MainViewModel(Action rowAnimation)
        {
            _rowAnimation = rowAnimation;
            var new_effects = new ObservableCollection<EffectViewModel>();
            new_effects.Add(new EffectViewModel() { Index = 00, Description = "description", Name = "Red" });
            new_effects.Add(new EffectViewModel() { Index = 01, Description = "description", Name = "Green" });
            new_effects.Add(new EffectViewModel() { Index = 02, Description = "description", Name = "Blue" });
            new_effects.Add(new EffectViewModel() { Index = 03, Description = "description", Name = "Cyan" });
            new_effects.Add(new EffectViewModel() { Index = 04, Description = "description", Name = "Magenta" });
            new_effects.Add(new EffectViewModel() { Index = 05, Description = "description", Name = "Yellow" });
            new_effects.Add(new EffectViewModel() { Index = 06, Description = "description", Name = "Orange" });
            new_effects.Add(new EffectViewModel() { Index = 07, Description = "description", Name = "White" });
            new_effects.Add(new EffectViewModel() { Index = 08, Description = "description", Name = "Dim" });
            new_effects.Add(new EffectViewModel() { Index = 09, Description = "description", Name = "Candy" });
            new_effects.Add(new EffectViewModel() { Index = 10, Description = "description", Name = "Sine" });
            new_effects.Add(new EffectViewModel() { Index = 11, Description = "description", Name = "Spark" });
            new_effects.Add(new EffectViewModel() { Index = 12, Description = "description", Name = "Pulse" });
            new_effects.Add(new EffectViewModel() { Index = 13, Description = "description", Name = "Solid" });
            EffectList = new_effects;
            CurrentEffect = EffectList[0];
        }

        public ICommand Animate
        {
            get
            {
                return new Command((o) =>
                {
                    _rowAnimation.Invoke();
                });
            }
        }

        public ObservableCollection<EffectViewModel> EffectList
        {
            get
            {
                return effects_;
            }
            set
            {
                if (effects_ != value)
                {
                    effects_ = value;
                    OnPropertyChanged();
                }
            }

        }

        public EffectViewModel CurrentEffect
        {
            get
            {
                return currentEffect_;
            }
            set
            {
                if (currentEffect_ != value)
                {
                    currentEffect_ = value;
                    //Log.Info($"EFFECT IS NOW {currentEffect_.Index}");
                    OnPropertyChanged();
                    SetNewColor();
                }
            }
        }

        public string DebugString
        {

            get { return debugString_; }
            set {
                debugString_ = value;
                OnPropertyChanged();
            }
        }

        public double SpeedLow
        {
            get
            {
                return speedLow_;
            }
            set
            {
                if (speedLow_ != value)
                {
                    speedLow_ = value;
                    if (SpeedHigh < speedLow_)
                    {
                        SpeedHigh = speedLow_;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public double SpeedHigh
        {
            get
            {
                return speedHigh_;
            }
            set
            {
                if (speedHigh_ != value)
                {
                    speedHigh_ = value;
                    if (SpeedLow > speedHigh_)
                    {
                        SpeedLow = speedHigh_;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public double RedLow
        {
            get
            {
                return redLow_;
            }
            set
            {
                if (redLow_ != value)
                {
                    redLow_ = value;
                    if (RedHigh < redLow_)
                    {
                        RedHigh = RedLow;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public double GreenLow
        {
            get
            {
                return greenLow_;
            }
            set
            {
                if (greenLow_ != value)
                {
                    greenLow_ = value;
                    if (GreenHigh < greenLow_)
                    {
                        GreenHigh = GreenLow;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public double BlueLow
        {
            get
            {
                return blueLow_;
            }
            set
            {
                if (blueLow_ != value)
                {
                    blueLow_ = value;
                    if (BlueHigh < BlueLow)
                    {
                        BlueHigh = BlueLow;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public double RedHigh
        {
            get
            {
                return redHigh_;
            }
            set
            {
                if (redHigh_ != value)
                {
                    redHigh_ = value;
                    if (RedLow > redHigh_)
                    {
                        RedLow = redHigh_;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public double GreenHigh
        {
            get
            {
                return greenHigh_;
            }
            set
            {
                if (greenHigh_ != value)
                {
                    greenHigh_ = value;
                    if (GreenLow > greenHigh_)
                    {
                        GreenLow = greenHigh_;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public double BlueHigh
        {
            get
            {
                return blueHigh_;
            }
            set
            {
                if (blueHigh_ != value)
                {
                    blueHigh_ = value;
                    if (BlueLow > blueHigh_)
                    {
                        BlueLow = blueHigh_;
                    }
                    else
                    {
                        SetNewColor();
                    }
                    OnPropertyChanged();
                }
            }
        }

        public Color ColorLow
        {
            get
            {
                return colorLow_;
            }
            set
            {
                if (colorLow_ != value)
                {
                    colorLow_= value;
                    OnPropertyChanged();
                }
            }
        }

        public Color ColorHigh
        {
            get
            {
                return colorHigh_;
            }
            set
            {
                if (colorHigh_ != value)
                {
                    colorHigh_ = value;
                    OnPropertyChanged();
                }
            }
        }

        bool SetString(bool v, int n, string name)
        {
            if (v != stringEnabled[n])
            {
                stringEnabled[n] = v;
                SetNewColor();
                OnPropertyChanged(name);
                return true;
            }
            return false;
        }

        public bool String1
        {
            get
            {
                return stringEnabled[0];
            }
            set
            {
                SetString(value, 0, "String1");
            }
        }

        public bool String2
        {
            get
            {
                return stringEnabled[1];
            }
            set
            {
                SetString(value, 1, "String2");
            }
        }

        public bool String3
        {
            get
            {
                return stringEnabled[2];
            }
            set
            {
                SetString(value, 2, "String3");
            }
        }

        public bool String4
        {
            get
            {
                return stringEnabled[3];
            }
            set
            {
                SetString(value, 3, "String4");
            }
        }

        public bool String5
        {
            get
            {
                return stringEnabled[4];
            }
            set
            {
                SetString(value, 4, "String5");
            }
        }

        public bool String6
        {
            get
            {
                return stringEnabled[5];
            }
            set
            {
                SetString(value, 5, "String6");
            }
        }

        void SetNewColor()
        {
            ColorLow = Color.FromRgb(RedLow, GreenLow, BlueLow);
            ColorHigh = Color.FromRgb(RedHigh, GreenHigh, BlueHigh);
            if (CurrentEffect != null)
            {
                byte rl = (byte)(ColorLow.R * 255.0);
                byte gl = (byte)(ColorLow.G * 255.0);
                byte bl = (byte)(ColorLow.B * 255.0);
                byte rh = (byte)(ColorHigh.R * 255.0);
                byte gh = (byte)(ColorHigh.G * 255.0);
                byte bh = (byte)(ColorHigh.B * 255.0);
                for (int i = 0; i < MainPage.max_strings; ++i)
                {
                    if (stringEnabled[i])
                    {
                        CurrentEffect.Params[i].set_color_range(0, rl, gl, bl, rh, gh, bh);
                        CurrentEffect.Params[i].set_speed_range(0, (ushort)(speedLow_ * 50), (ushort)(speedHigh_ * 50));
                        CurrentEffect.Set(i);
                    }
                }
            }
        }
    }
}
