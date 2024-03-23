using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Xamarin.Forms;

namespace PoseMate_App
{
    public partial class MainPage : ContentPage
    {
        public MainPage()
        {
            InitializeComponent();
        }
        private async void PoseTracker_Clicked(object sender, EventArgs e)
        {
            // WWhen the button is clciked , the second page is brought up
            await Navigation.PushAsync(page: new PoseTracker()); 
            
        }

        private void MoreStats_Clicked(object sender, EventArgs e)
        {
            DisplayAlert("Button 2 Clicked", "Button 2 was clicked!", "OK");
        }

        private void Graph_Clicked(object sender, EventArgs e)
        {
            DisplayAlert("Button 3 Clicked", "Button 3 was clicked!", "OK");
        }
    }
}
