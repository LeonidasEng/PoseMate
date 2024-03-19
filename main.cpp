#include "mbed.h"
#include "mpu6050.h"
#include <cstdint>

extern I2C i2c;
MPU6050 mpu6050;
AnalogIn potentiometer(A0); // A0
AnalogIn pulseSensor(A1); // A1
// Reset button connected to NRST pin
DigitalOut power_LED(D6); // D6
DigitalOut activity_LED(D7); // D7
DigitalOut pulse_LED(D8); // D8
PwmOut buzzer(D9); // D9
Serial pc(USBTX, USBRX);
Serial bt(PA_9, PA_10); // D1, D0

Timer t, dt;
Timer range;
Ticker ticker;
bool ledState = false;
int calib_Count;


// MPU6050 sensor variables
extern float aRes, gRes;
extern float ax, ay, az;
extern float gx, gy, gz;
extern float accelBias[3], gyroBias[3];
extern float yaw, pitch, roll;
extern float deltat, q[4], SelfTest[6], temperature;
extern int16_t accelCount[3], gyroCount[3], tempCount;

int delt_t = 0; // used to control display output rate
int count = 0;  // used to control display output rate
int lastUpdate = 0, firstUpdate = 0, Now = 0;  // Integration time for filter

// Pulse sensor variables
float pulseValue = 0;

// Potentiometer variables
bool tonePlaying = false;
float potValue = 0; // Read from pot
int8_t potPercentage = 0; // Calculate percentage pot value
float lastPotValue = 0;

// Modes
bool enabledSensorMode = false;
bool enabledMeasureMode = false;
bool enabledFormMode = false;
char formModeSub = '0';

void setup();
void toggle_PowerLED();
void Bluetooth();

void play_Tone(float frequency, float duration);
void stop_Tone();
void pot_measure(float &lastpotValue);

float pulse();

// IMU Functions
void WhoAMI_MPU6050();
void activeMPU6050();
void allSensorData();

float measureMode(AnalogIn& pot, float inchesMin, float inchesMax);
void formMode();

int main() {
    
    setup(); // Initialise timers, tickers, comm protocols
     
    float lastPotValue = 0; // Initialise last pot value

    WhoAMI_MPU6050(); // Initialise and calibrate IMU

    // Start up sound
    play_Tone(1047, 250);
    thread_sleep_for(300);
    play_Tone(1319, 250);
    thread_sleep_for(300);
    play_Tone(1568, 250);
    thread_sleep_for(200);

    // Phone app used - Serial Bluetooth Terminal on Android
    bt.printf("Welcome to PoseMate!\n");
    bt.printf("Select from the following options\n");
    bt.printf("1 - ALL SENSOR DATA\n");
    bt.printf("2 - MEASURE ARM MODE\n");
    bt.printf("3 - IMPROVE FORM MODE\n");
    bt.printf("0 - STOP OUTPUT\n");
    
    // Active data mode
    while(true) 
    {
        
        power_LED = 1;
        activity_LED = !activity_LED; // Flash activity LED at the start of each loop
        Bluetooth();

        // Active sensors
        pot_measure(lastPotValue);
        pulse(); 
        
        // Mode selection
        if (enabledSensorMode) 
        {
            allSensorData();
        } else if (enabledMeasureMode) {
            float inches = measureMode(potentiometer, 14.0, 20.0);
            pc.printf("Circumference: %.2f inches, Percentage %d\n", inches, potPercentage);
            bt.printf("Circumference: %.2f inches, Percentage %d\n", inches, potPercentage);
            thread_sleep_for(100);
        } else if (enabledFormMode)
        {
            formMode();
        }     
    }
}

void setup()
{
    pc.baud(115200);
    bt.baud(9600);
    i2c.frequency(400000); 
    ticker.attach(&toggle_PowerLED, 0.5);
    t.start();
    dt.start();    
}

void WhoAMI_MPU6050()
{
    // Read the WHO_AM_I register, this is a good test of communication
    uint8_t whoami = mpu6050.readByte(MPU6050_ADDRESS, WHO_AM_I_MPU6050);  // Read WHO_AM_I register for MPU-6050
    bt.printf("I AM 0x%x\n", whoami); 
    bt.printf("I SHOULD BE 0x68\n");

    if (whoami == 0x68) // WHO_AM_I should always be 0x68
    {  
        pc.printf("MPU6050 initialised...\n");
        thread_sleep_for(1000);
        mpu6050.MPU6050SelfTest(SelfTest); // Start by performing self test and reporting values
        thread_sleep_for(1000);

        if(SelfTest[0] < 1.0f && SelfTest[1] < 1.0f && SelfTest[2] < 1.0f && SelfTest[3] < 1.0f && SelfTest[4] < 1.0f && SelfTest[5] < 1.0f) 
        {
            mpu6050.resetMPU6050(); // Reset registers to default for device calibration
            bt.printf("Self test complete.\n");
            mpu6050.calibrateMPU6050(gyroBias, accelBias);
            bt.printf("Accel Bias: %f, %f, %f Gyro Bias: %f, %f, %f\n", accelBias[0], accelBias[1], accelBias[2], gyroBias[0], gyroBias[1], gyroBias[2]);
            mpu6050.initMPU6050();
            bt.printf("MPU6050 initialised for active data mode...\n");
            thread_sleep_for(1000);
        } else 
        {
            bt.printf("Device did not pass self-test!\n");
        }
    } else {
        bt.printf("Could not connect to MPU6050: ");
        bt.printf("%#x\n", whoami);

        while(1);
    }
}

void activeMPU6050()
{
    // If data ready bit set, all data registers have new data
    if(mpu6050.readByte(MPU6050_ADDRESS, INT_STATUS) & 0x01) 
    {  
        mpu6050.readAccelData(accelCount); 
        mpu6050.getAres();

        // Accelerometer in G's
        ax = (float)accelCount[0]*aRes - accelBias[0];  // get actual g value, this depends on scale being set
        ay = (float)accelCount[1]*aRes - accelBias[1];   
        az = (float)accelCount[2]*aRes - accelBias[2];  

        mpu6050.readGyroData(gyroCount);
        mpu6050.getGres();

        // Gyroscope in °/s
        gx = (float)gyroCount[0]*gRes; //- gyroBias[0];  // get actual gyro value, tends to drift keep disabled.
        gy = (float)gyroCount[1]*gRes; //- gyroBias[1];  
        gz = (float)gyroCount[2]*gRes; //- gyroBias[2];   


        tempCount = mpu6050.readTempData();
        temperature = (tempCount) / 340. + 36.53; // Temperature conversion to Centigrade
        

        Now = dt.read_us();
        deltat = (float)((Now - lastUpdate)/1000000.0f) ; // integration time set based on last filter update
        lastUpdate = Now;

        // Sensor fusion of accel and gyro
        mpu6050.MadgwickFilter(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f); // Gyro rate in radians per second for quaternion calculation

        // Can switch to yaw, pitch, roll depending on orientation of MPU6050 in wristwatch
        yaw   = atan2(-2.0f * q[1] * q[2] + 2 * q[0] * q[3], q[2] * q[2] - q[3] * q[3] - q[1] * q[1] + q[0] * q[0]);   
        roll = -asin(2.0f * q[2] * q[3] + 2 * q[0] * q[1]);
        pitch  = atan2(-2.0f * q[1] * q[3] + 2 * q[0] * q[2], q[3] * q[3] - q[2] * q[2] - q[1] * q[1] + q[0] * q[0]);       
        yaw *= 180.0f / PI; // Back to degrees per second now quaternion operation is complete
        roll *= 180.0f / PI; 
        pitch  *= 180.0f / PI;
    }
}

void allSensorData() 
{
    activeMPU6050();
    
    delt_t = dt.read_ms() - count;
    if (delt_t > 100) // update every 100ms
    { 
        float elapsed_time = t.read(); // Read elapsed time in seconds
        int total_seconds = elapsed_time;
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;
        int seconds = total_seconds % 60;
        int milliseconds = (elapsed_time - total_seconds) * 1000;

        /* PC Serial Output */
        pc.printf("%02d:%02d:%02d:%03d, %.2f, %.2f, %.2f, %.2f, %d, %.2f\n",
            hours, minutes, seconds, milliseconds,
            yaw, pitch, roll, temperature, potPercentage, pulseValue);

        /* Bluetooth Serial Output */
        bt.printf("%02d:%02d:%02d:%03d, %.2f, %.2f, %.2f, %.2f, %d, %.2f\n",
            hours, minutes, seconds, milliseconds,
            yaw, pitch, roll, temperature, potPercentage, pulseValue);
        count = dt.read_ms();
    }
}

void stop_Tone() {
    buzzer = 0; // Stop the buzzer
    tonePlaying = false;
    ticker.detach(); // Stop the ticker
}

void play_Tone(float frequency, float duration) {
    if(tonePlaying) {
        return; // A tone is already playing, avoid new one
    }
    tonePlaying = true;
    buzzer.period(1.0 / frequency);
    buzzer = 0.5; // Start the buzzer
    
    float durationInSeconds = duration / 1000.0;
    ticker.attach(&stop_Tone, durationInSeconds);
}

void toggle_PowerLED() {
    if (calib_Count < 10) { //  5 seconds as 500ms * 10
        power_LED = !power_LED; // Toggle the LED
        calib_Count++; // Increment the counter
    } else {
        ticker.detach(); // Stop the ticker after the calibration phase
        power_LED = 1; // Ensure LED is ON after calibration
    }
}

void pot_measure(float &lastPotValue) 
{
    static bool withinRange = false;
    potValue = potentiometer.read(); // Read the value of the pot
    potPercentage = static_cast<uint8_t>(potValue * 100);    
    // Full contraction
    if (potPercentage > 90 && lastPotValue <= 90) {
        // Play a high tone for engaged bicep
        play_Tone(1400.0, 1500);

    // Finding pump zero
    } else if (potPercentage >= 18 && potPercentage <= 22)
    {
        if (!withinRange)
        {
            range.reset();
            range.start();
            withinRange = true;
        }
        if (range.read_ms() >= 3000)
        {
            play_Tone(1500, 150);
            thread_sleep_for(50);
            play_Tone(1500, 150);
            thread_sleep_for(50);
            play_Tone(1500, 150);
            thread_sleep_for(50);    
        }     
    } else {
        // Update lastPotValue if NOT 0% or 100%
        lastPotValue = potPercentage;
        range.stop();
        range.reset();
        withinRange = false;
    }
}

float measureMode(AnalogIn& pot, float inchesMin, float inchesMax) 
{
    float input = pot.read();

    return inchesMin + (inchesMax - inchesMin) * (input - 0.0) / (1.0 - 0.0);
}

void formMode()
{
    activeMPU6050();

    delt_t = dt.read_ms() - count;
    if (delt_t > 100) // update every 100ms
    { 

        float toleranceRoll = 0;
        static bool modeSelectionMessage = false;

        switch(formModeSub) 
        {
            case 'c': // Curl mode
                toleranceRoll = 20;
                modeSelectionMessage = false; // Reset message flag when a valid mode is selected
                break;
            case 'p': // Pose mode
                toleranceRoll = 50;
                modeSelectionMessage = false; // Reset message flag when a valid mode is selected
                break;
            default:
                if (!modeSelectionMessage) {
                    bt.printf("SELECT FORM MODE: 'c' for Curl OR 'p' for Pose\n");
                    modeSelectionMessage = true; // Prevent repeating the message
                }
                return; // Exit if no valid mode is selected
        }

        // Check if yaw and roll are within tolerance
        bool yawCorrect = yaw > -90;
        bool rollCorrect = fabs(roll) <= toleranceRoll;

        if (!yawCorrect || !rollCorrect) {
            play_Tone(110, 1000); // Play tone to indicate correction is needed
        } else {
            stop_Tone(); // Stop tone if within tolerance
        }

        // Output to PC and Bluetooth for debugging or user feedback
        pc.printf("Mode: %c, Yaw: %.2f, Roll: %.2f\n", formModeSub, yaw, roll);
        bt.printf("Mode: %c, Yaw: %.2f, Roll: %.2f\n", formModeSub, yaw, roll);
        count = dt.read_ms();
    }

}

void Bluetooth()
{
    if (bt.readable()) {
        char command = bt.getc();
        switch (command) {
            case '1':
                enabledSensorMode = true; // Enable sensor data output
                break;
            // Add a case for disabling the output, could use another character, e.g., '0'
            case '0':
                enabledSensorMode = false;
                enabledMeasureMode = false;
                enabledFormMode = false;
                formModeSub = '0';
                bt.printf("Select from the following options\n");
                bt.printf("1 - ALL SENSOR DATA\n");
                bt.printf("2 - MEASURE ARM MODE\n");
                bt.printf("3 - IMPROVE FORM MODE\n");
                bt.printf("0 - STOP OUTPUT\n");
                break;
            case '2':
                enabledMeasureMode = true;

                break;
            case '3':
                enabledFormMode = true;
                break;
            case 'c': // Curl mode
            case 'p': // Pose mode
                if (enabledFormMode) {
                    formModeSub = command;
                } else {
                    bt.printf("Enable form mode first\n");
                }
                break;
            case '\n': // Newline character
            case '\r': // Carriage return character
                // Ignore these characters
                break;
            default:
                bt.printf("Invalid command\n");
                break;
        }
    }
}

float pulse()
{
    pulseValue = pulseSensor.read();
    
    if (pulseValue > 0.5) 
    {
        pulse_LED = 1;
    } else {
        pulse_LED = 0;
    }
    return pulseValue;
}