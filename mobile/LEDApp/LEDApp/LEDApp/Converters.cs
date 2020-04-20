using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using Xamarin.Forms;

namespace LEDApp
{
    public class ModeToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (Equals(value, null))
                return new GridLength(0);

            int mode = (int)value;

            switch (mode)
            {
                // mode 0, show listview
                case 0:
                    {
                        return new GridLength(1, GridUnitType.Auto);
                    }
                // mode else, hide listview
                default:
                    {
                        return new GridLength(0);
                    }
            }
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException("Only one way bindings are supported with this converter");
        }
    }

    public class ModeToInvisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (Equals(value, null))
                return new GridLength(0);

            int mode = (int)value;

            switch (mode)
            {
                // mode 0, hide listview
                case 0:
                    {
                        return new GridLength(0);
                    }
                // mode else, show listview
                default:
                    {
                        return new GridLength(1, GridUnitType.Auto);
                    }
            }
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException("Only one way bindings are supported with this converter");
        }
    }
}
