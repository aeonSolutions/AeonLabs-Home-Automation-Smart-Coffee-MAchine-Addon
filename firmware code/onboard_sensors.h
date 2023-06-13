/*
 Copyright (c) 2023 Miguel Tomas, http://www.aeonlabs.science

License Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
You are free to:
   Share — copy and redistribute the material in any medium or format
   Adapt — remix, transform, and build upon the material

The licensor cannot revoke these freedoms as long as you follow the license terms. Under the following terms:
Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. 
You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

NonCommercial — You may not use the material for commercial purposes.

ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under
 the same license as the original.

No additional restrictions — You may not apply legal terms or technological measures that legally restrict others
 from doing anything the license permits.

Notices:
You do not have to comply with the license for elements of the material in the public domain or where your use
 is permitted by an applicable exception or limitation.
No warranties are given. The license may not give you all of the permissions necessary for your intended use. 
For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.


Before proceeding to download any of AeonLabs software solutions for open-source development
 and/or PCB hardware electronics development make sure you are choosing the right license for your project. See 
https://github.com/aeonSolutions/PCB-Prototyping-Catalogue/wiki/AeonLabs-Solutions-for-Open-Hardware-&-Source-Development
 for Open Hardware & Source Development for more information.

*/

#include "SparkFunLSM6DS3.h"
#include "mserial.h"
#include "interface_class.h"
#include <Wire.h>
//#include "src/sensors/aht20.h"
#include "src/sensors/sht3x.h"

#ifndef ONBOARD_SENSORS_DEF
  #define ONBOARD_SENSORS_DEF

class ONBOARD_SENSORS {
  private:

  public:
    INTERFACE_CLASS* interface=nullptr;
    mSerial* mserial= nullptr;

    //AHT20 sensor  **********************************
    bool AHTsensorAvail;
    float ONBOARD_TEMP, ONBOARD_HUMIDITY;
    uint8_t HT_SENSOR_ADDRESS;
    
    //AHT20_SENSOR* onboardTHsensor;
    SHT3X_SENSOR* onboardTHsensor;

    // LSM6DS3 motion sensor  ******************************
    uint8_t LSM6DS3_ADDRESS= 0x6B; // default address is 0x6B
    uint8_t IMU_CS_IO =38;
    bool MotionSensorAvail;
    LSM6DS3 LSM6DS3sensor = LSM6DS3( I2C_MODE, LSM6DS3_ADDRESS);
    float LSM6DS3_errors, LSM6DS3_Motion_X, LSM6DS3_Motion_Y, LSM6DS3_Motion_Z, LSM6DS3_GYRO_X,  LSM6DS3_GYRO_Y,  LSM6DS3_GYRO_Z,  LSM6DS3_TEMP;

    bool MOTION_DETECT_EN;
    float ROLL_THRESHOLD ; 
    time_t $espunixtimePrev;
    float prevReadings[6];
    uint8_t numtimesBeforeDetectMotion;
    uint8_t numtimesMotionDetected;

    uint8_t NUMBER_OF_ONBOARD_SENSORS;

    // I2C scanner  ************************************
    // see https://stackoverflow.com/questions/52221727/arduino-wire-library-returning-error-code-7-which-is-not-defined-in-the-library
    char*i2c_err_t[9];

    ONBOARD_SENSORS();
    void I2Cscanner();
    void init(INTERFACE_CLASS* interface, mSerial* mserial);

    void startLSM6DS3();
    void request_onBoard_Sensor_Measurements();
    void getLSM6DS3sensorData();
    bool motionDetect();
    bool motionShakeDetected(uint8_t numShakes);

    bool gbrl_commands(String $BLE_CMD, uint8_t sendTo);
    bool helpCommands(uint8_t sendTo);
    void initRollTheshold();
};

#endif