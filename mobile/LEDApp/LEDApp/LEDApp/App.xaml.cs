using Xamarin.Forms;

namespace LEDApp
{
    public partial class App : Application
	{
		public App ()
		{
			InitializeComponent();
			MainPage = new LEDApp.MainPage();
		}

		protected override void OnStart ()
		{
		}

		protected override void OnSleep ()
		{
		}

		protected override void OnResume ()
		{
		}
	}
}
