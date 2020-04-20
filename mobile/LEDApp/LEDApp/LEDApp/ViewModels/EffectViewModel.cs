namespace LEDApp
{
    public class EffectViewModel : BaseViewModel
    {
        int index_;         // effect #
        string name_;
        string description_;

        const int max_strings = 6;

        Parameters[] params_ = new Parameters[] {
            new Parameters(),
            new Parameters(),
            new Parameters(),
            new Parameters(),
            new Parameters(),
            new Parameters()
        };

        public EffectViewModel() : base()
        {
            for(int i=0; i< max_strings; ++i)
            {
                params_[i].reset(i);
            }
        }

        public Parameters[] Params => params_;

        public string Description
        {
            get
            {
                return description_;
            }
            set
            {
                description_ = value;
            }
        }

        public string Name
        {
            get
            {
                return name_;
            }
            set
            {
                if (name_ != value)
                {
                    name_ = value;
                    OnPropertyChanged();
                }
            }
        }

        public int Index
        {
            get
            {
                return index_;
            }
            set
            {
                index_ = value;
            }
        }

        public override string ToString()
        {
            return Name;
        }

        public void Set(int string_id)
        {
            var p = params_[string_id];
            p.set_effect(Index);
            MainPage.udp[string_id].set_state(p);
        }
    }

}
