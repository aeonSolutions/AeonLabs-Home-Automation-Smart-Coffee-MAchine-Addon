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

#include "onboard_sensors.h"
#include "interface_class.h"
#include "onboard_led.h"
#include "m_math.h"

ONBOARD_SENSORS::ONBOARD_SENSORS(){
    this->NUMBER_OF_ONBOARD_SENSORS=0;
    this->AHT20_ADDRESS = 0x38;
}

void ONBOARD_SENSORS::init(INTERFACE_CLASS* interface, mSerial* mserial){
    this->mserial=mserial;
    this->mserial->printStrln("\nInit onboard sensors...");
    
    this->interface=interface;
    this->i2c_err_t[0]="I2C_ERROR_OK";
    this->i2c_err_t[1]="I2C_ERROR_DEV";     // 1
    this->i2c_err_t[2]="I2C_ERROR_ACK";     // 2
    this->i2c_err_t[3]="I2C_ERROR_TIMEOUT"; // 3
    this->i2c_err_t[4]="I2C_ERROR_BUS";     // 4
    this->i2c_err_t[5]="I2C_ERROR_BUSY";    // 5
    this->i2c_err_t[6]="I2C_ERROR_MEMORY";  // 6
    this->i2c_err_t[7]="I2C_ERROR_CONTINUE";// 7
    this->i2c_err_t[8]="I2C_ERROR_NO_BEGIN"; // 8

    this->aht20 = new AHT20_SENSOR();
    this->aht20->init(this->interface, this->AHT20_ADDRESS);
    this->aht20->startAHT();

    this->startLSM6DS3();
    this->prevReadings[0] = 0.0;
    this->prevReadings[1] = 0.0;
    this->prevReadings[2] = 0.0;
    this->prevReadings[3] = 0.0;
    this->prevReadings[4] = 0.0;
    this->prevReadings[5] = 0.0;
    this->ROLL_THRESHOLD  = 8.5;   
    this->numtimesBeforeDetectMotion=5;
    this->numtimesMotionDetected=0;

    time(&this->$espunixtimePrev);
    this->mserial->printStrln("done.");
}

// ***************************************************************
void ONBOARD_SENSORS::request_onBoard_Sensor_Measurements(){
    this->aht20->requestMeasurements();

    this->aht_temp = aht20->measurement[0];
    this->aht_humidity = aht20->measurement[1];

    getLSM6DS3sensorData();
}


void ONBOARD_SENSORS::startLSM6DS3(){
  if ( LSM6DS3sensor.begin() != 0 ) {
    this->mserial->printStrln("\nError starting the motion sensor at specified address (0x"+String(this->LSM6DS3_ADDRESS, HEX)+")");
    interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
    interface->onBoardLED->statusLED(100,2); 
    this->MotionSensorAvail=false;
  } else {
    this->mserial->printStr("Starting Motion Sensor...");
    interface->onBoardLED->led[0] = interface->onBoardLED->LED_GREEN;
    interface->onBoardLED->statusLED(100,0); 
    this->NUMBER_OF_ONBOARD_SENSORS=this->NUMBER_OF_ONBOARD_SENSORS+1;
    this->MotionSensorAvail = true; 
    this->mserial->printStrln("done.");
  }
}

// ********************************************************
void ONBOARD_SENSORS::getLSM6DS3sensorData() {
  if (MotionSensorAvail==false){
    startLSM6DS3();
  }
      

  this->prevReadings[0]=LSM6DS3_Motion_X;
  this->prevReadings[1]=LSM6DS3_Motion_Y;
  this->prevReadings[2]=LSM6DS3_Motion_Z;
  this->prevReadings[3]=LSM6DS3_GYRO_X;
  this->prevReadings[4]=LSM6DS3_GYRO_Y;
  this->prevReadings[5]=LSM6DS3_GYRO_Z;   

    //Get all parameters

  LSM6DS3_Motion_X=LSM6DS3sensor.readFloatAccelX();
  LSM6DS3_Motion_Y=LSM6DS3sensor.readFloatAccelY();
  LSM6DS3_Motion_Z=LSM6DS3sensor.readFloatAccelZ();
  LSM6DS3_GYRO_X=LSM6DS3sensor.readFloatGyroX();
  LSM6DS3_GYRO_Y=LSM6DS3sensor.readFloatGyroY();
  LSM6DS3_GYRO_Z=LSM6DS3sensor.readFloatGyroZ();
  LSM6DS3_TEMP=LSM6DS3sensor.readTempC();
  //mserial->printStrln(String(LSM6DS3sensor.readTempF()));
  LSM6DS3_errors=LSM6DS3sensor.allOnesCounter;
  LSM6DS3_errors=LSM6DS3sensor.nonSuccessCounter;
}


// ***********************************************************
bool ONBOARD_SENSORS::motionShakeDetected(uint8_t numShakes){
  time_t timeNow;

  time(&timeNow);

  if (timeNow - this->$espunixtimePrev < 60){
    if ( this->motionDetect() ) {
      if (this->numtimesMotionDetected >= numShakes){
        this->numtimesMotionDetected =0;
        this->$espunixtimePrev=timeNow;
        return true;
      }else{
        this->numtimesMotionDetected ++;
      }
    }
  }else{
    this->numtimesMotionDetected =0;
    this->$espunixtimePrev=timeNow;
  }
  return false;
};

// **********************************************
void ONBOARD_SENSORS::initRollTheshold(){
    this->mserial->printStr("Calibration of motion detection.Dont move the device..");
    float X, Y, Z, totalAccel;
    for (int i=0; i<100; i++) {
      X += this->LSM6DS3_Motion_X;
      Y += this->LSM6DS3_Motion_Y;
      Z += this->LSM6DS3_Motion_Z;
      delay(1);
    }
    X /= 10;
    Y /= 10;
    Z /= 10;

    totalAccel = sqrt(X*X + Y*Y + Z*Z);
    this->ROLL_THRESHOLD = totalAccel * ( 1.0 + this->interface->config.MOTION_SENSITIVITY); 
    this->mserial->printStrln("Done.\n");
}

// ************************************************************
bool ONBOARD_SENSORS::motionDetect(){
  time_t timeNow;
  long int $timedif;

  time(&timeNow);
  if ( ( timeNow - this->$espunixtimePrev) > 1 && (timeNow != this->interface->$espunixtimePrev) ){ 
    this->getLSM6DS3sensorData();

    // ToDo: prev Time 

    float X, Y, Z, totalAccel;
    for (int i=0; i<10; i++) {
      X += this->LSM6DS3_Motion_X;
      Y += this->LSM6DS3_Motion_Y;
      Z += this->LSM6DS3_Motion_Z;
      delay(1);
    }
    X /= 10;
    Y /= 10;
    Z /= 10;

    totalAccel = sqrt(X*X + Y*Y + Z*Z);

   // Serial.println("Accel : "+String(totalAccel));

    if (totalAccel > this->ROLL_THRESHOLD) {
      return true;
    } else{
      return false;
    }
  }
}
// *************************************************************

void ONBOARD_SENSORS::I2Cscanner() {
  this->mserial->printStrln ("I2C scanner. \n Scanning ...");  
  uint8_t count = 0;
  String addr;

  for (uint8_t i = 8; i < 120; i++){

    Wire.beginTransmission (i);          // Begin I2C transmission Address (i)
    uint8_t error = Wire.endTransmission();
    addr="";
    if (i < 16){
       addr="0";
    }
    addr=addr+String(i, HEX);

    if (error == 0) { // Receive 0 = success (ACK response)
      this->mserial->printStr ("Found address: ");
      this->mserial->printStr (String(i, DEC));
      this->mserial->printStr (" (0x");
      this->mserial->printStr (String(i, HEX));     // PCF8574 7 bit address
      this->mserial->printStrln (")");
      count++;
    } else if (error == 4) {
      this->mserial->printStr("Unknown error at address 0x");
      this->mserial->printStrln(addr);
      interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
      interface->onBoardLED->statusLED(100,1); 
    } else{
    interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
    interface->onBoardLED->led[1] = interface->onBoardLED->LED_RED;
    interface->onBoardLED->statusLED(100,2); 
    }
  }

  this->mserial->printStr ("Found ");
  this->mserial->printStr (String(count));        // numbers of devices
  this->mserial->printStrln (" device(s).");
  if (count == 0) {
      interface->onBoardLED->led[1] = interface->onBoardLED->LED_RED;
      interface->onBoardLED->statusLED(100,2); 
    }else if (count==2){
      interface->onBoardLED->led[1] = interface->onBoardLED->LED_GREEN;
      interface->onBoardLED->led[1] = interface->onBoardLED->LED_RED;
      interface->onBoardLED->statusLED(100,2); 
    }else{
    interface->onBoardLED->led[1] = interface->onBoardLED->LED_GREEN;
    interface->onBoardLED->statusLED(100,2); 
  }
}

// *********************************************************
 bool ONBOARD_SENSORS::helpCommands(uint8_t sendTo){
    String dataStr="Onboard sensors commands:\n" \
                    "$sensor port [on/off]           - Enable/ Disable Power on External Ports\n" \
                    "\n" \
                    "$ms view                        - View motion detection sensitivity\n" \
                    "$ms set [0;1]                   - Set motion detection sensitivity\n" \                    
                    "$ot                             - View Onboard Temperature data ( " + String(char(176)) + String("C )") +  " )\n" \
                    "$oh                             - View Onboard Humidity data ( % )\n" \
                    "\n" \
                    "$oa                             - View onboard acceleartion data ( g ) \n" \
                    "$og                             - View Onboard Gyroscope data ( dps ) \n" \
                    "$om                             - View Microcontroller Temperature ( " + String(char(176)) + String("C )") + " )\n\n";


    this->interface->sendBLEstring( dataStr, sendTo);

    return false; 
 }

// *********************************************************
bool ONBOARD_SENSORS::gbrl_commands(String $BLE_CMD, uint8_t sendTo){
  String dataStr="";
  
  if($BLE_CMD.indexOf("$ms ")>-1){
    if( $BLE_CMD == "$ms view" ){
      dataStr = "Current motion sensitivity is " + String(this->interface->config.MOTION_SENSITIVITY) + "\n";
    }else if ($BLE_CMD.indexOf("$ms set ")>-1){
      String value= $BLE_CMD.substring(8, $BLE_CMD.length());
      dataStr = "$ CMD ERR";
      if (isNumeric(value)){
          double val = (double) value.toDouble();
          if(val>0.0 and val<1.0){
            this->interface->config.MOTION_SENSITIVITY = val;
            dataStr = "new motion sensitivity set: " + String(this->interface->config.MOTION_SENSITIVITY) + "\n";       
          }
      }
    }else{
      dataStr = "$ CMD ERR";
    }
    this->interface->sendBLEstring( dataStr, sendTo); 
    return true; 

  }else if($BLE_CMD=="$?" || $BLE_CMD=="$help"){
      return this->helpCommands(sendTo);
  } else if($BLE_CMD=="$ot"){
    this->request_onBoard_Sensor_Measurements();
    dataStr=String("Current onboard Temperature is: ") + String(roundFloat(this->aht_temp,2)) + String(char(176)) + String("C") +String(char(10));
    this->interface->sendBLEstring( dataStr, sendTo); 
    return true; 
  } else if($BLE_CMD=="$oh"){
    this->request_onBoard_Sensor_Measurements();
    dataStr=String("Current onboard Humidity is: ") + String(this->aht_humidity)+ String(" %") +String(char(10));
    this->interface->sendBLEstring( dataStr, sendTo);  
    return true; 
  } else if($BLE_CMD=="$oa"){
    this->request_onBoard_Sensor_Measurements();
    dataStr=String("Current onboard acceleration data ( g ) : ") + String("  X: ")+String(roundFloat(this->LSM6DS3_Motion_X,2)) +"  Y:" + String(roundFloat(this->LSM6DS3_Motion_Y,2)) + "  Z: " + String(roundFloat(this->LSM6DS3_Motion_Z,2)) +String(char(10));
    this->interface->sendBLEstring( dataStr, sendTo);  
    return true; 
  } else if($BLE_CMD=="$og"){
    this->request_onBoard_Sensor_Measurements();
    dataStr=String("Current onboard angular data ( dps ): ") + String("  X: ") + String(roundFloat(this->LSM6DS3_GYRO_X,2)) +"  Y:" + String(roundFloat(this->LSM6DS3_GYRO_Y,2)) + "  Z: " + String(roundFloat(this->LSM6DS3_GYRO_Z,2)) +String(char(10));
    this->interface->sendBLEstring( dataStr, sendTo);  
    return true; 
  } else if($BLE_CMD=="$om"){
    this->request_onBoard_Sensor_Measurements();
    dataStr=String("Current MCU temperature is: ") + String(roundFloat(this->LSM6DS3_TEMP,2))+ String(char(176)) + String("C")  +String(char(10));
    this->interface->sendBLEstring( dataStr, sendTo);  
    return true; 
  }else if($BLE_CMD.indexOf("$sensor port")>-1){
    if( $BLE_CMD == "$sensor port on" ){
      digitalWrite(this->interface->EXT_PLUG_PWR_EN ,HIGH);
      this->interface->POWER_PLUG_IS_ON=true;
      dataStr="Power Enabled on Sensor Ports";
      this->interface->sendBLEstring( dataStr, sendTo);  
      return true;
    }else if( $BLE_CMD == "$sensor port off" ){
      digitalWrite(this->interface->EXT_PLUG_PWR_EN ,LOW);
      this->interface->POWER_PLUG_IS_ON=false;
      dataStr="Power Disabled on Sensor Ports";
      this->interface->sendBLEstring( dataStr, sendTo);  
      return true;
    }
  }

  return false; 
}


// ******************************************************