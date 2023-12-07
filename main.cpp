/* PoseMate Microcontroller Source Code
 * Copyright (c) 2023 LeonidasEng
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "BufferedSerial.h"
#include "MPU6050.h"


MPU6050 mpu6050(0x68, AFS_2G, GFS_250DPS, PB_7, PB_6);
BufferedSerial pc(USBTX, USBRX);
Timer t;



int main()
{

    t.start(); // Start Timer for output

    // Setup
    pc.set_baud(115200);
    char buffer[128]; // Buffer for formatted strings

    // Read the WHO_AM_I register
    uint8_t whoami = mpu6050.getWhoAmI();
    snprintf(buffer, sizeof(buffer), "I am 0x%x\n\rI should be 0x68\n\r", whoami);
    printf("%s", buffer);

    if (!mpu6050.init()) {
        printf("MPU6050 initialisation failed\n\r");
        return -1; // Non-zero term for stderr
    }

    if (whoami == 0x68) 
    {
        printf("IMU MPU6050 connected...\n\r");
        ThisThread::sleep_for(1s);

        if (mpu6050.selfTestOK()) 
        {
            printf("Self test passed.\n\r");
            mpu6050.reset();
            mpu6050.calibrate();
            printf("MPU6050 calibrated and initialised.\n\r");
            ThisThread::sleep_for(1s);    
        } else {
            printf("Device did not pass the self-test!\n\r");
            return -1; 
        }
    } else {
        printf("Could not connect to the MPU6050: \n\r");
        printf("%x \n", whoami);

        while(1); // Loop forever if communication is not established.
    }
    // Loop
    while(1)
    {
        if (mpu6050.dataReady()) {
            mpu6050.accel(); // Read 2g acceleration data
            mpu6050.gyro(); // Read 250° gyroscope data

            // Displaying sensor values
            float ax = mpu6050.accelData[0];
            float ay = mpu6050.accelData[1];
            float az = mpu6050.accelData[2];

            float gx = mpu6050.gyroData[0];
            float gy = mpu6050.gyroData[1];
            float gz = mpu6050.gyroData[2];

            // Read temperature
            float temperature = mpu6050.temp();

            // Calculate Roll and Pitch - Found issue with Euler equation we now have Gimbal lock!
            float pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180 / M_PI;
            float roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / M_PI;

            auto elapsed_time = t.elapsed_time();

            // Print values to serial console - Now includes Time
            snprintf(buffer, sizeof(buffer), 
                 
                 // Seperated by comma for CSV and C# visualiser
                "%02d:%02d:%02d:%03d, %.2f, %.2f, %.2f\n\r",
                int (elapsed_time / 1h), int((elapsed_time % 1h) / 1min), int((elapsed_time % 1min) / 1s), int((elapsed_time % 1s) / 1ms),
                pitch, roll, temperature);
                
                // Raw values:
                //"[%02d:%02d:%02d,%03d] Accel: (X: %.2f, Y: %.2f, Z: %.2f) G | Gyro: (A: %.2f, B: %.2f, C: %.2f) °/s | Temperature: %.2f°C\n\r", 
                //int (elapsed_time / 1h), int(elapsed_time % 1h / 1min), int(elapsed_time % 1min / 1s), int(elapsed_time % 1s / 1ms),
                //ax, ay, az, gx, gy, gz, temperature);

            printf("%s", buffer);

            ThisThread::sleep_for(100ms); // Delay to read data
        }
    }
}
