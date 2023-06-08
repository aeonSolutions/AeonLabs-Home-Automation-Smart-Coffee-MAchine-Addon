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

#include "interface_class.h"
#include "Arduino.h"
#include "m_file_functions.h"
#include "mserial.h"
#include <WiFiMulti.h>
#include "lang_base.h"
#include "lang_12daq.h"

INTERFACE_CLASS::INTERFACE_CLASS(){
  this->firmware_version="-.-.-";
  
  // ___________ MCU freq ____________________
  this->SAMPLING_FREQUENCY = 80; 
  this->MAX_FREQUENCY=240;
  this->WIFI_FREQUENCY = 80; // min WIFI MCU Freq is 80-240
  this->MIN_MCU_FREQUENCY = 40;
  this->SERIAL_DEFAULT_SPEED = 115200;

    // I2C IOs  __________________________
  this->I2C_SDA_IO_PIN = 8;
  this->I2C_SCL_IO_PIN = 9;
  
  // Power Saving ____________________________________
  this->LIGHT_SLEEP_EN=false;

  // External Ports IO Pin assignment ________________________________
  this-> EXT_PLUG_PWR_EN = 2;
  this->BATTERY_ADC_IO= 1;

}

void INTERFACE_CLASS::init( mSerial* mserial, bool DEBUG_ENABLE) {  
  this->mserial=mserial;
  this->mserial->DEBUG_EN=DEBUG_ENABLE;

  this->BL = Pangodream_18650_CL( this->BATTERY_ADC_IO, 1.7, 20);

  this->loadDefaultLanguagePack( );

  this->mserial->printStrln ("Firmware Build: " + String(__DATE__ ) + " " + String(__TIME__) + " version: " + this->firmware_version + "\n");
  //this->rtc.setTime(1609459200);  // 1st Jan 2021 00:00:00
  this->rtc.offset = this->config.gmtOffset_sec; // change offset value
  this->rtc.setTimeStruct( CompileDateTime(String(__DATE__ ).c_str(), String(__TIME__).c_str() ) ); 

}

// ***********************************************************************
void INTERFACE_CLASS::init_BLE(BLECharacteristic *pCharacteristicTX){
  this->pCharacteristicTX=pCharacteristicTX;
  this->mserial->pCharacteristicTX=pCharacteristicTX;
}

// **************************************************
void INTERFACE_CLASS::settings_defaults(){
  this->$espunixtime = time(0);
  this->$espunixtimePrev = time(0);
  this->$espunixtimeStartMeasure= time(0);

  this->WIFI_FREQUENCY=80 ; // min WIFI MCU Freq is 80-240
  this->MIN_MCU_FREQUENCY=10;
  
  this->SERIAL_DEFAULT_SPEED=115200;    

  this->DATA_VALIDATION_KEY = "A9CD7F1B6688159B54BBE862F638FF9D29E0FA5F87C69D27BFCD007814BA69C9";

  this->config.isBatteryPowered=false;
   
  this->Measurments_EN=false;
  this->Measurments_NEW=false;
  this->config.SENSOR_DATA_FILENAME="measurements.csv";
  this->config.DEVICE_PASSWORD="";
  
  this->config.MOTION_SENSITIVITY = 0.07;
  this->config.language ="en";

  this->LIGHT_SLEEP_EN=false;
  
  this->mserial->printStrln("settings defaults loaded.");
}
// ****************************************************
  bool INTERFACE_CLASS::setMCUclockFrequency(int clockFreq){
    xSemaphoreTake(this->McuFreqSemaphore, portMAX_DELAY);
      this->McuFrequencyBusy = true;
        this->UARTserial->flush();
        
        setCpuFrequencyMhz(Freq);
        this->CURRENT_CLOCK_FREQUENCY=Freq;
        if (Freq < 80) {
          this->MCU_FREQUENCY_SERIAL_SPEED = 80 / Freq * this->SERIAL_DEFAULT_SPEED;
        } else {
          this->MCU_FREQUENCY_SERIAL_SPEED = this->SERIAL_DEFAULT_SPEED;
        }
        
        this->UARTserial->end();  
        delay(300);
        this->UARTserial->begin(this->MCU_FREQUENCY_SERIAL_SPEED);
        this->UARTserial->print("The current Serial Baud speed on the UART Port is ");
        this->UARTserial->println(this->MCU_FREQUENCY_SERIAL_SPEED);

      this->mserial->printStrln("Setting MCU Freq to  " + String(getCpuFrequencyMhz()) + "MHz" );
      this->McuFrequencyBusy = false;
    xSemaphoreGive(this->McuFreqSemaphore);
    return true;
  };

// ***************************************************************
    String INTERFACE_CLASS::DeviceTranslation(String key){
      if ( this->deviceLangJson.isNull() == false ){
        if(this->deviceLangJson.containsKey(key)){
          return String( this->deviceLangJson[key].as<char*>() );
        }else{
          return key + " not found"; 
        }
      }else{
        return "JSON is null"; 
      }
    }
// ..................................................
    String INTERFACE_CLASS::BaseTranslation(String key){
      if ( this->baseLangJson.isNull() == false ){
        if(this->baseLangJson.containsKey(key)){
          return String (this->baseLangJson[key].as<char*>() );
        }else{
          return key + " not found"; 
        }
      }else{
        return "JSON is null"; 
      }
    }


// ***************************************************************************
void INTERFACE_CLASS::sendBLEstring(String message,  uint8_t sendTo ){  
  if( message == "")
      message= "empty message found.\n";


  if ( sendTo == mSerial::DEBUG_TO_BLE || sendTo == mSerial::DEBUG_ALL_USB_UART_BLE || sendTo == mSerial::DEBUG_TO_BLE_UART ){
    if (WiFi.status() == WL_CONNECTED){
      WiFi.disconnect(true);
      delay(100);
    }
  }
  this->mserial->printStr(message, mSerial::DEBUG_TYPE_INFO, sendTo);
}

// --------------------------------------------------------------------------

bool INTERFACE_CLASS::saveSettings(fs::FS &fs){
  this->config.DEBUG_EN = this->mserial->DEBUG_EN; // ON / OFF
  this->config.DEBUG_TO = this->mserial->DEBUG_TO; // UART, BLE   
  this->config.DEBUG_TYPE = this->mserial->DEBUG_TYPE; // Verbose // Errors 
  this->config.DEBUG_SEND_REPOSITORY = this->mserial->DEBUG_SEND_REPOSITORY ;
  
  this->mserial->printStrln("Saving the Smart Device settings...");

  if (fs.exists("/settings.cfg") )
    fs.remove("/settings.cfg");

  File settingsFile = fs.open("/settings.cfg", FILE_WRITE); 
  if ( !settingsFile ){
    Serial.println("error creating settings file.");
    settingsFile.close();
    return false;
  }

  settingsFile.print( String(this->config.gmtOffset_sec) + String(';'));
  settingsFile.print( String(this->config.daylightOffset_sec) + String(';'));

  settingsFile.print( String(this->config.isBatteryPowered) + String(';'));

  settingsFile.print( String(this->config.POWER_PLUG_ALWAYS_ON) + String(';'));

  settingsFile.print( String(this->config.onboard_motion_sensor_en) + String(';'));

  settingsFile.print( String(this->config.onboard_temperature_en) + String(';'));

  settingsFile.print( String(this->config.onboard_humidity) + String(';'));

  settingsFile.print( String(this->config.DEBUG_EN) + String(';'));

  settingsFile.print( String(this->config.DEBUG_TO) + String(';'));

  settingsFile.print( String(this->config.DEBUG_TYPE) + String(';'));

  settingsFile.print( String(this->config.DEBUG_SEND_REPOSITORY) + String(';'));

  settingsFile.print( this->config.SENSOR_DATA_FILENAME + String(';'));

  settingsFile.print( this->config.DEVICE_PASSWORD + String(';'));

  settingsFile.print( String(this->config.MOTION_SENSITIVITY ,3)+ String(';'));

  settingsFile.print( this->config.language + String(';'));

  settingsFile.close();
  return true;
}

// ********************************************************************************
bool INTERFACE_CLASS::loadSettings(fs::FS &fs){
  this->mserial->printStr("Loading the Smart Device settings...");
  
  File settingsFile = fs.open("/settings.cfg", FILE_READ);
  if (!settingsFile){
    this->mserial->printStrln("not found.");
    settingsFile.close();
    return false;
  }
  if (settingsFile.size() == 0){
    this->mserial->printStrln("Invalid file");
    settingsFile.close();
    return false;    
  }

  File settingsFile2 = fs.open("/settings.cfg", FILE_READ);
 // Serial.println("full contents:");
 // Serial.println( settingsFile2.readString() );
 // Serial.println(" === END ===== ");
        
this->mserial->printStrln("done. Size: " + String( settingsFile.size()) );

    this->config.gmtOffset_sec = atol(settingsFile.readStringUntil( ';' ).c_str() ); 

    String temp = settingsFile.readStringUntil(';');
    this->config.daylightOffset_sec = temp.toInt();

// ******************* Power management **************
    temp= settingsFile.readStringUntil(';');
    this->config.isBatteryPowered = *(temp.c_str()) != '0';
    temp= settingsFile.readStringUntil(';');
    this->config.POWER_PLUG_ALWAYS_ON = *(temp.c_str()) != '0';

// ********************** onboard sensors *********************
    temp= settingsFile.readStringUntil(';');
    this->config.onboard_motion_sensor_en = *(temp.c_str()) != '0';

    temp= settingsFile.readStringUntil(';');
    this->config.onboard_temperature_en = *(temp.c_str()) != '0';

    temp= settingsFile.readStringUntil(';');
    this->config.onboard_humidity = *(temp.c_str()) != '0';

    // ************* DEBUG *****************
    temp= settingsFile.readStringUntil(';');
    this->config.DEBUG_EN = *(temp.c_str()) != '0';

    temp = settingsFile.readStringUntil(';');
    this->config.DEBUG_TO = temp.toInt();

    temp = settingsFile.readStringUntil(';');
    this->config.DEBUG_TYPE = temp.toInt();

    temp= settingsFile.readStringUntil(';');
    this->config.DEBUG_SEND_REPOSITORY = *(temp.c_str()) != '0';

    this->config.SENSOR_DATA_FILENAME = settingsFile.readStringUntil(';');

    this->config.DEVICE_PASSWORD = settingsFile.readStringUntil(';');

    this->config.MOTION_SENSITIVITY = settingsFile.readStringUntil(';').toDouble();
    
    this->config.language = settingsFile.readStringUntil(';');
  
  settingsFile.close();

  this->mserial->DEBUG_EN = this->config.DEBUG_EN ; 
  this->mserial->DEBUG_TO = this->config.DEBUG_TO; 
  this->mserial->DEBUG_TYPE = this->config.DEBUG_TYPE; 
  this->mserial->DEBUG_SEND_REPOSITORY = this->config.DEBUG_SEND_REPOSITORY ;

  if (this->config.language == "en"){
    this->loadDefaultLanguagePack( );
  }else{

    this->loadBaseLanguagePack( this->config.language,  mSerial::DEBUG_ALL_USB_UART_BLE );
    this->loadDeviceLanguagePack( this->config.language,  mSerial::DEBUG_ALL_USB_UART_BLE );
  
  }
  return true;
}
// *************************************************************

bool INTERFACE_CLASS::loadDefaultLanguagePack(uint8_t sendTo ){
  String dataStr = "";
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(this->baseLangJson, defaultBaseTranslations);
  if (error){
      dataStr="(397) Failed loading base language file into memory (" + String(error.f_str()) + "). \n";
      this->mserial->printStrln(dataStr, mSerial::DEBUG_TYPE_ERRORS, sendTo);
      return false;
  }

    // Deserialize the JSON document
  error = deserializeJson(this->deviceLangJson, default_SCC_translations);
  if (error){
      dataStr="(434) Failed loading the device language file into memory (" + String(error.f_str()) + "). \n";
      this->mserial->printStrln(dataStr, mSerial::DEBUG_TYPE_ERRORS, sendTo);
      return true;
  }

  return true;
}

// ************************************************************

bool INTERFACE_CLASS::loadBaseLanguagePack(String country, uint8_t sendTo ){
  String dataStr= "";
  // loading language file for the base code 
  String filename = "/lang/" + country  +"_sys.lang";
  if ( false == LittleFS.exists( filename) ) {
    dataStr="(377) Base language file not found. \n";
    this->sendBLEstring( dataStr,  sendTo );  
    return false;
  }

  File baseLang = LittleFS.open(filename, FILE_READ);
  if( baseLang == false){
      dataStr="(384) Failed access to the base language file. \n";
      this->sendBLEstring( dataStr,  sendTo );  
      return false;
  }

  String file_content = baseLang.readString();
  baseLang.close();

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(this->baseLangJson, file_content);
  if (error){
      dataStr="(397) Failed loading base language file into memory (" + String(error.f_str()) + "). \n";
      this->sendBLEstring( dataStr,  sendTo );  
      this->mserial->printStrln("Lang contents: \""+ file_content +"\"", mSerial::DEBUG_TYPE_VERBOSE, mSerial::DEBUG_ALL_USB_UART_BLE);

      return false;
  }

  return true;

}

// *****************************************************

bool INTERFACE_CLASS::loadDeviceLanguagePack(String country, uint8_t sendTo ){
  String dataStr= "";
   // loading language file for the smart device 
  String filename = "/lang/" + country  +".lang";
  if ( false == LittleFS.exists( filename) ) {
    dataStr="(414) device language file not found. \n";
    this->sendBLEstring( dataStr,  sendTo );  
    return true;
  }

  File deviceLang = LittleFS.open(filename, FILE_READ);
  if( deviceLang == false){
      dataStr="(421) Failed access to the device language file. \n";
      this->sendBLEstring( dataStr,  sendTo );  
      return true;
  }

  String file_content = deviceLang.readString();
  deviceLang.close();

  // Deserialize the JSON document
  DeserializationError error2 = deserializeJson(this->deviceLangJson, file_content);
  if (error2){
      dataStr="(434) Failed loading the device language file into memory (" + String(error2.f_str()) + "). \n";
      this->sendBLEstring( dataStr,  sendTo );  
      return true;
  }
  
  return true;
}
