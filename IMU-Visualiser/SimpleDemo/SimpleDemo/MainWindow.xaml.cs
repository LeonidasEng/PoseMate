using System;
using System.IO.Ports;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Markup;
using System.IO;
using System.Windows.Threading;
using Microsoft.Win32;
using System.Threading;

namespace SimpleDemo
{

    public partial class MainWindow : Window
    {
        private SerialPort serialPort;
        private StringBuilder receivedDataBuffer = new StringBuilder();

        private string? CSVPathToFile = string.Empty; // string is nullable so multiple tests can be completed

        private List<string> dataList = new List<string>();
        private DateTime testStartTime;
        private DispatcherTimer updateTimer;

        private float yaw, pitch, roll, temperature;


        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = new MainViewModel();      // Added for transform functionality
            serialPort = new SerialPort("COM3", 115200); // Verify correct COM port in Device Manager
                                                         // Baud rate 115200 for USB connection.
            SetupSerial();                               // Call setup for the Serial connection.

            updateTimer = new DispatcherTimer();        // For measuring the time on the UI textbox.
            updateTimer.Interval = TimeSpan.FromSeconds(1); // Update every second
            updateTimer.Tick += UpdateTimer_Tick;

            this.Closing += MainWindow_Closing;
        }

        private void SetupSerial()
        {
            try
            {
                serialPort.Open(); // Only one serial port instance can be open. Ensure it's closed in mbed
                serialPort.DataReceived += new SerialDataReceivedEventHandler(DataReceivedHandler); // On background thread
                txtOutput.Text = "Serial port is connected";
            }
            catch (Exception ex)
            {
                txtOutput.Text = "Serial port is NOT connected! " + ex.Message; // Displays exception if open in other monitor or not connected
            }
        }
        private void UpdateTimer_Tick(object? sender, EventArgs e)
        {
            TimeSpan duration = DateTime.Now - testStartTime;
            txtOutput.Text = $"Starting Test: {System.IO.Path.GetFileName(CSVPathToFile)} running for {duration.ToString(@"hh\:mm\:ss")}\n";
        }

        //**********************************************************************************************************
        private void btnStartTest_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(CSVPathToFile))
            {
                txtOutput.Text = "Please enter a valid file path.";
                return;
            }
            dataList.Clear(); // Clear previous data
            testStartTime = DateTime.Now; // Set the start time
            updateTimer.Start(); // Start updating the duration display
        }

        private void btnEndTest_Click(object sender, RoutedEventArgs e)
        {

            updateTimer.Stop(); // Stop updating the duration display

            TimeSpan finalDuration = DateTime.Now - testStartTime;
            txtOutput.Text = $"Test {System.IO.Path.GetFileName(CSVPathToFile)} has been saved and ran for {finalDuration.ToString(@"hh\:mm\:ss")}";


            SaveDataToCsv(); // You can change the file name and path as needed
        }

        private void SaveDataToCsv()
        {
            if (!string.IsNullOrEmpty(CSVPathToFile))
            {
                try
                {
                    using (StreamWriter sw = new StreamWriter(CSVPathToFile))
                    {
                        // Write header for CSV
                        sw.WriteLine("TIMESTAMP, YAW, PITCH, ROLL, TEMPERATURE");

                        // Writes data for CSV
                        foreach (var dataEntry in dataList)
                        {
                            sw.WriteLine(dataEntry);
                        }
                    }
                }
                catch (Exception ex)
                {
                    txtOutput.Text = "Error saving data to CSV file: " + ex.Message;
                }
                CSVPathToFile = null; // Reset the file path

            }
            else
            {
                txtOutput.Text = "No file path selected. Please select a file path to save the CSV.";
            }
        }

        private void btnSaveToCSV_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = "CSV File: (*.csv)|*.csv";
            if (saveFileDialog.ShowDialog() == true)
            {
                CSVPathToFile = saveFileDialog.FileName;

            }
        }
        //**********************************************************************************************************
        private void MainWindow_Closing(object? sender, System.ComponentModel.CancelEventArgs e)
        {

            if (serialPort != null && serialPort.IsOpen)
            {
                serialPort.Close();
            }

        }


        private void DataReceivedHandler(object sender, SerialDataReceivedEventArgs e) // sender is SerialPort and related data event
        {
            // Dispatcher needs to be run in current - Use BeginInvoke rather than Invoke to prevent dead lock.
            Application.Current.Dispatcher.BeginInvoke(new Action(() =>                      // WPF requires we use UI thread for updates to UI elements
            {
                try
                {
                    string receivedData = serialPort.ReadExisting(); // Instead of using BytesToRead we use ReadExisting to read available data
                    receivedDataBuffer.Append(receivedData);         // Using StringBuilder instead of Read() to gather new data with incomplete data
                                                                     // Partial monitoring works well


                    string buffer = receivedDataBuffer.ToString();
                    if (buffer.Contains("\n"))
                    {

                        string[] lines = buffer.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);
                        foreach (string line in lines)
                        {
                            if (!string.IsNullOrWhiteSpace(line) && line.Contains(",")) // Ignores empty and looks for lines containing commas
                            {
                                string[] items = line.Split(','); // split line string into items based on comma delimiter

                                if (items.Length == 5) // Must be 5 items
                                {

                                    string[] timeParts = items[0].Split(":"); // Need to be able to show mbed output as TimeSpan on UI, another split gets around formatting errors

                                    if (timeParts.Length == 4)
                                    {
                                        // Create an array of timeParts before reinserting into new TimeSpan object
                                        int hours = int.Parse(timeParts[0]);
                                        int minutes = int.Parse(timeParts[1]);
                                        int seconds = int.Parse(timeParts[2]);
                                        int milliseconds = int.Parse(timeParts[3]);

                                        TimeSpan timestamp = new TimeSpan(0, hours, minutes, seconds, milliseconds); // TimeSpan has param for days so we set that to zero

                                        yaw = float.Parse(items[1]);
                                        pitch = float.Parse(items[2]);
                                        roll = float.Parse(items[3]);
                                        temperature = float.Parse(items[4]);

                                        // Added to Test data collection
                                        TimeSpan duration = DateTime.Now - testStartTime; // Shows duration of test and not time of operation
                                        string dataEntry = $"{duration.ToString(@"hh\:mm\:ss\.fff")},{yaw}, {pitch}, {roll}, {temperature}";
                                        dataList.Add(dataEntry);

                                        // Set the pitch and roll properties on main view model
                                        var viewModel = DataContext as MainViewModel;
                                        if (viewModel != null)
                                        {
                                            viewModel.Yaw = yaw;
                                            viewModel.Pitch = pitch;
                                            viewModel.Roll = roll;
                                        }
                                        // Ensure text boxes are not null before setting content
                                        if (txtPitchValue != null && txtRollValue != null)
                                        {
                                            txtTimestamp.Content = timestamp.ToString("hh\\:mm\\:ss\\.fff");
                                            txtYawValue.Content = yaw.ToString("0.00°");
                                            txtPitchValue.Content = pitch.ToString("0.00°");
                                            txtRollValue.Content = roll.ToString("0.00°");
                                            txtTemp.Content = temperature.ToString("0.00°C");
                                        }
                                        receivedDataBuffer.Clear(); // Clears buffer
                                    }
                                    else
                                    {
                                        txtTimestamp.Content = "Invalid TimeSpan";
                                    }
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    txtOutput.Text = "Error processing data from serial port: " + ex.Message;
                }
            }));
        }
    }
}
