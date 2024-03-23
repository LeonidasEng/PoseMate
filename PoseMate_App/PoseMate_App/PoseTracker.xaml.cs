using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Xamarin.Forms;
using Xamarin.Forms.Xaml;
using Plugin.BluetoothClassic;
using Plugin.BluetoothClassic.Abstractions;

namespace PoseMate_App
{
	[XamlCompilation(XamlCompilationOptions.Compile)]
	public partial class PoseTracker : ContentPage
	{
        // Bluetooth Classic adapter
       //IBluetoothAdapter bluetoothAdapter;


        public PoseTracker ()
		{
			InitializeComponent ();
		}

        private void UpdateValuesButton_Clicked(object sender, EventArgs e)
        {
            // Randaom value generator as placeholder for Bluetooth
            Random random = new Random();
            int pitchValue = random.Next(100, 999);
            int rollValue = random.Next(100, 999);
            int pulseValue = random.Next(100, 999);

            // Display updated values
            PitchLabel.Text = pitchValue.ToString();
            RollLabel.Text = rollValue.ToString();
            PulseLabel.Text = pulseValue.ToString();

            // Display pitch value and message based on criteria
            LargePitchLabel.Text = pitchValue.ToString();

            if (pitchValue > 500)
            {
                MessageLabel.Text = "Pitch value exceeds the threshold.";
            }
            else
            {
                MessageLabel.Text = "Pitch value is below the threshold.";
            }
        }
    }
}
