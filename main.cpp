/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "BufferedSerial.h"
#include "MPU6050.h"


//I2C i2c(PB_7, PB_6);
//Timer t;
MPU6050 mpu6050(0x68, AFS_2G, GFS_250DPS, PB_7, PB_6);
BufferedSerial pc(USBTX, USBRX);



int main()
{
    // Setup
    pc.set_baud(115200);

    uint8_t whoami = mpu6050.getWhoAmI();
    printf("I am 0x%x\n\r", whoami);
    printf("I should be 0x68\n\r");

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
            mpu6050.gyro(); // Read 250d gyroscope data

            // Displaying sensor values
            float ax = mpu6050.accelData[0];
            float ay = mpu6050.accelData[1];
            float az = mpu6050.accelData[2];

            float gx = mpu6050.gyroData[0];
            float gy = mpu6050.gyroData[1];
            float gz = mpu6050.gyroData[2];

            // Read temperature
            float temperature = mpu6050.temp();

            // Print values to serial console
            printf("Accel: (X: %f, Y: %f, Z: %f) G", ax, ay, az);
            printf("| Gyro: (A: %f, B: %f, C: %f) °/s\n\r", gx, gy, gz);
            printf("Temperature: %f°C\n\r", temperature);

            ThisThread::sleep_for(100ms); // Delay to read data
        }
    }
}
