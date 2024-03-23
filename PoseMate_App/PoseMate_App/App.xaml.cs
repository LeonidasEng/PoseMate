using System;
using Xamarin.Forms;
using Xamarin.Forms.Xaml;

namespace PoseMate_App
{
    public partial class App : Application
    {
        public App()
        {
            InitializeComponent();
            // Creation of  a navigation page to the second page of the app.
            NavigationPage PoseTracker = new NavigationPage(new MainPage());
            MainPage = PoseTracker;
        }

        protected override void OnStart()
        {
        }

        protected override void OnSleep()
        {
        }

        protected override void OnResume()
        {
        }
    }
}
