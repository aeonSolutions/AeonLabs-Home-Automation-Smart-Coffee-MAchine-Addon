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

#include "gbrl.h"
#include "Arduino.h"
#include"m_math.h"
#include "FS.h"
#include <LittleFS.h>
#include "m_atsha204.h"

GBRL::GBRL() {

}

void GBRL::init(INTERFACE_CLASS* interface, M_WIFI_CLASS* mWifi){
  this->interface=interface;
  this->interface->mserial->printStr("\ninit GBRL ...");
  this->mWifi= mWifi;
  this->interface->mserial->printStrln("done.");
}

// *********************************************************
 bool GBRL::helpCommands(uint8_t sendTo ){
    String dataStr="GBRL commands:\n" \
                    "$help $?                           - View available GBRL commands\n" \
                    "$dt                                - Device Time\n" \
                    "$ver                               - Device Firmware Version\n" \
                    "$did                               - This Smart Device Unique Serial Number\n" 
                    "$sleep [on/off]                    - Enable/disable device sleep (save battery power)\n" 
                    "$sleep status                      - View current device sleep status\n" 
                    "\n" \
                    "$firmware update                   - Update the Device with a newer Firmware\n" \
                    "$firmware cfg [on/auto/manual]     - Configure Firmware updates\n" \
                    "\n" \
                    "$settings reset                    - reset Settings to Default values\n" \
                    "$set pwd [password]                - Set device access password\n" \
                    "\n" \
                    "$debug [on/off] [ble/ uart / all]  - Output Debug\n" \
                    "$debug status                      - View Debug cfg\n" \
                    "debug repository [on/off]          - Save Debug data in the data repository\n" \
                    "$debug verbose [on/off]            - Output all debug messages\n" \
                    "$debug errors [on/off]             - Output only Error messages\n" \
                    "\n" \
                    "$lang dw [country code]            - Download a language pack to the smart device (requires internet conn.)\n" \
                    "$lang set [country code]           - Change the smart device language\n\n";

    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return false; 
 }
// ******************************************************************************************

bool GBRL::commands(String $BLE_CMD, uint8_t sendTo ){
  String dataStr="";
  if($BLE_CMD.indexOf("$lang dw ")>-1){
    return this->set_device_language($BLE_CMD, sendTo);
  }

  // ToDo
  if($BLE_CMD.indexOf("$lang set ")>-1){
    dataStr = "command under development. Update to the Next revision of the firmware.";
    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true;
  }

  if($BLE_CMD.indexOf("$sleep ")>-1){
    if($BLE_CMD == "$sleep status" ){
        if ( this->interface->LIGHT_SLEEP_EN == true){
          dataStr= "Sleep mode is enabled on this device.\n\n" ;
        }else{
          dataStr= "Sleep mode is disabled on this device.\n\n" ;
        }

        this->interface->sendBLEstring( dataStr,  sendTo ); 
        return true;
    }else if($BLE_CMD == "$sleep on" ){
        this->interface->LIGHT_SLEEP_EN = true;
        dataStr= "Smart Device Sleep mode enabled." + String( char(10));
        this->interface->sendBLEstring( dataStr,  sendTo ); 
        return true;
    }else if($BLE_CMD == "$sleep off" ){
        this->interface->LIGHT_SLEEP_EN = false;
        dataStr= "Smart Device Sleep mode disabled." + String( char(10));
        this->interface->sendBLEstring( dataStr,  sendTo ); 
        return true;
    }else{
        dataStr="UNK $ CMD \r\n";
        this->interface->sendBLEstring( dataStr,  sendTo ); 
        return true;
    }
  }

  if($BLE_CMD == "$did"){
    dataStr = "This Smart Device Serial Number is : ";
    dataStr += CryptoICserialNumber(this->interface) + "\n";
    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true;
  }else if($BLE_CMD=="$plug status"){
      return this->plug_status( sendTo );
  }else if($BLE_CMD=="$?" || $BLE_CMD=="$help"){
      return  this->helpCommands( sendTo );
  }else if($BLE_CMD.indexOf("$battery ")>-1){
      return this->powerManagement($BLE_CMD,  sendTo );
  }else if($BLE_CMD=="$settings reset"){
      if (LittleFS.exists("/settings.cfg") ){
        if(LittleFS.remove("/settings.cfg") > 0 ){
          this->interface->mserial->printStrln("old settings file deleted");          
        }else{
          this->interface->sendBLEstring( "unable to delete settings\n",  sendTo );  
        }
      }else{
        this->interface->mserial->printStrln("settings file not found");
      }
    this->interface->settings_defaults();
    this->interface->sendBLEstring( "Settings were reseted to default values.\n",  sendTo );  
    return true;
  }else if($BLE_CMD=="$ver"){
      dataStr = "Version " + String(this->interface->firmware_version) + "\n";
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;

  }else if($BLE_CMD.indexOf("$usb ")>-1){
      return this->powerManagement($BLE_CMD,  sendTo );            

  }else if($BLE_CMD=="$dt"){
      dataStr=String(this->interface->rtc.getDateTime(true)) + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return true;
  }else if($BLE_CMD.indexOf("$debug ")>-1){
      return this->debug_commands($BLE_CMD,  sendTo );
  } else if ( $BLE_CMD.indexOf("$set pwd ")>-1 ){
    if (this->interface->Measurments_EN){
      dataStr="Passowrd Cannot be set while running measurments\n\n";
    }else{
      String value = $BLE_CMD.substring(9, $BLE_CMD.length());
      this->interface->config.DEVICE_PASSWORD=value;
      dataStr = "password accepted\n\n";
    }
    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true;
  }

  return false;
}


// ******************************************************
bool GBRL::plug_status(uint8_t sendTo ){
  String dataStr="";
  dataStr="ADC Plug Status:" + String(char(10));
  dataStr+= "Always ON 3.3V: " + String( this->interface->config.POWER_PLUG_ALWAYS_ON ?  "Yes": "No") + String(char(10));
  dataStr+= "Power is currently " + String( this->interface->POWER_PLUG_IS_ON ? "ON": "OFF") + String( char(10));

  this->interface->sendBLEstring( dataStr,  sendTo );
  return true;
}


// ********************************************************
bool GBRL::runtime(uint8_t sendTo ){
    long int hourT; 
    long int minT; 
    long int secT; 
    long int daysT;
    String dataStr="";
    long int $timedif;
    time_t now = time(nullptr);

    $timedif = time(&now)- this->interface->$espunixtimeStartMeasure;
    hourT = (long int) ($timedif/3600);
    minT = (long int) ($timedif/60 - (hourT*60));
    secT =  (long int) ($timedif - (hourT*3600) - (minT*60));
    daysT = (long int) (hourT/24);
    hourT = (long int) (($timedif/3600) - (daysT*24));
    
    dataStr="RunTime"+ String(char(10)) + String(daysT)+"d "+ String(hourT)+"h "+ String(minT)+"m "+ String(secT)+"s "+ String(char(10));
    this->interface->sendBLEstring( dataStr, sendTo );
    return true;
}





// *********************************************************

bool GBRL::debug_commands(String $BLE_CMD, uint8_t sendTo ){
String dataStr="";

 if($BLE_CMD=="$debug repository on"){
      this->interface->mserial->DEBUG_SEND_REPOSITORY=1;
      dataStr= "Debug to a data repository enabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;
  }else if($BLE_CMD=="$debug repository off"){
      this->interface->mserial->DEBUG_SEND_REPOSITORY=0;
      dataStr= "Debug to a data repository disabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;
  }else if($BLE_CMD=="$debug status"){
      if (this->interface->mserial->DEBUG_EN){
        dataStr= "Debug is ON ";

        if ( this->interface->mserial->DEBUG_TO == this->interface->mserial->DEBUG_TO_BLE ){
          dataStr += "BLE ";
        }else if  ( this->interface->mserial->DEBUG_TO == this->interface->mserial->DEBUG_TO_UART ) {
          dataStr += "UART " ;
        }else{
          dataStr += "BLE & UART " ;   
        }

        if (this->interface->mserial->DEBUG_TYPE == this->interface->mserial->DEBUG_TYPE_VERBOSE){
          dataStr += ", Mode: verbose" + String( char(10));
        }else{
          dataStr += ", Mode: errors only" + String( char(10));
        }

        if (this->interface->mserial->DEBUG_SEND_REPOSITORY == 0){
          dataStr += "upload Logs to repository is: OFF " + String( char(10));
        }else{
          dataStr += "upload Logs to repository is: ON " + String( char(10));
        }

      }else{
        dataStr= "Debug is OFF" + String( char(10));
      }
      this->interface->sendBLEstring( dataStr + "\n",  sendTo );     
      return true;

  }else if($BLE_CMD=="$debug verbose"){
      this->interface->mserial->DEBUG_TYPE= this->interface->mserial->DEBUG_TYPE_VERBOSE;
      dataStr= "Debug Verbose enabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo );   
      return true; 

  }else if($BLE_CMD=="$debug errors"){
      this->interface->mserial->DEBUG_TYPE= this->interface->mserial->DEBUG_TYPE_ERRORS;
      dataStr= "Debug only Errors enabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo );      
      return true;

  }else if($BLE_CMD.indexOf("$debug ")>-1){
    if($BLE_CMD.indexOf("$debug off")>-1){
      this->interface->mserial->DEBUG_EN=0;
      dataStr= "Debug disabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;

    } else if($BLE_CMD=="$debug on ble"){
      this->interface->mserial->DEBUG_EN = 1;
      this->interface->mserial->DEBUG_TO = this->interface->mserial->DEBUG_TO_BLE;
      dataStr= "Debug to BLE enabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return true;

    }else if($BLE_CMD=="$debug on uart"){
      this->interface->mserial->DEBUG_EN = 1;
      this->interface->mserial->DEBUG_TO= this->interface->mserial->DEBUG_TO_UART;
      dataStr= "Debug to UART enabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return true;

    }else if($BLE_CMD=="$debug on all"){
      this->interface->mserial->DEBUG_EN = 1;
      this->interface->mserial->DEBUG_TO = this->interface->mserial->DEBUG_TO_BLE_UART;
      dataStr= "Debug to UART & BLE enabled" + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return true;
    }
  }
  return false;
}


// *********************************************************

bool GBRL::powerManagement(String $BLE_CMD, uint8_t sendTo ){
  String dataStr="";

  if($BLE_CMD.indexOf("$battery ")>-1){
    String value= $BLE_CMD.substring(9, $BLE_CMD.length());
    if (value.indexOf("ON")>-1){
      // usb OFF  BAT ON 
      this->interface->config.isBatteryPowered=true;
      
    } else if (value.indexOf("OFF")>-1){
        // USB ON BAT OFF
          this->interface->config.isBatteryPowered=false;
    }
  } else if($BLE_CMD=="$BAT" || $BLE_CMD=="$bat"){
      this->interface->mserial->printStr("Value from pin: ");
      this->interface->mserial->printStrln(String(analogRead(this->interface->BATTERY_ADC_IO)));
      this->interface->mserial->printStr("Average value from pin: ");
      this->interface->mserial->printStrln(String(this->interface->BL.pinRead()));
      this->interface->mserial->printStr("Volts: ");
      this->interface->mserial->printStrln(String(this->interface->BL.getBatteryVolts()));
      this->interface->mserial->printStr("Charge level: ");
      this->interface->mserial->printStrln(String(this->interface->BL.getBatteryChargeLevel()));
      this->interface->mserial->printStrln("");
      
      dataStr="Battery at "+ String(this->interface->BL.getBatteryChargeLevel()) + "%" + String(char(10));
      this->interface->sendBLEstring( dataStr,  sendTo );
      return true;
  } else if($BLE_CMD.indexOf("$usb ")>-1){
    String value= $BLE_CMD.substring(5, $BLE_CMD.length());
    if (value.indexOf("ON")>-1){
      // usb ON  BAT OFF 
          this->interface->config.isBatteryPowered=false;
    } else if (value.indexOf("OFF")>-1){
        // USB OFF BAT ON
          this->interface->config.isBatteryPowered=true;
    }else{
      dataStr="UNK $ CMD \r\n";
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;
    }
  }else{
      dataStr="UNK $ CMD \r\n";
      this->interface->sendBLEstring( dataStr,  sendTo );
      return true; 
  } 

  if (this->interface->saveSettings()){
    dataStr="$ CMD OK \n";
    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true; 
  }else{
    dataStr="Error saving settings \n";
    this->interface->sendBLEstring( dataStr,  sendTo );  
    return true;
  }

  return false;
}




// ****************************************
 bool GBRL::set_device_language(String $BLE_CMD, uint8_t sendTo){
  String dataStr="";

  if($BLE_CMD.indexOf("$lang dw ")<0){
    return false;
  }

  String country = $BLE_CMD.substring(9, $BLE_CMD.length());
  if (country.length()>3){
    dataStr="Invalid country code requested \n";
    this->interface->sendBLEstring( dataStr,  sendTo );  
    return true;
  }

  File directory = LittleFS.open("/lang");

    if( directory.isDirectory() == false ){
    if(LittleFS.mkdir("/lang") == false){
      dataStr="(421) Error creating lang directory. \n";
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return true;
    }else{
      dataStr="(425) Language Pack directory created \n";
      this->interface->sendBLEstring( dataStr,  sendTo );  
    }
  }
    
  directory.close();

  dataStr="Initializing Language pack download (1/2)... \n";
  this->interface->sendBLEstring( dataStr,  sendTo );  

 // Request file from the base code repo
  String filename = country  +".lang";
  this->interface->mserial->printStrln("filename to request: "+filename); //, mSerial::DEBUG_TYPE_VERBOSE, mSerial::DEBUG_ALL_USB_UART_BLE);

  if (LittleFS.exists( "/lang/" + country  +"_sys.lang" ) )
    LittleFS.remove( "/lang/" + country  +"_sys.lang" );


  String httpAddr = "https://github.com/aeonSolutions/aeonlabs-ESP32-C-Base-Firmware-Libraries/raw/main/translation/" + filename;
  if ( false == this->mWifi->downloadFileHttpGet("/lang/" + country  +"_sys.lang", httpAddr, sendTo) ){
    dataStr="Error Installing base language file \n";
    this->interface->sendBLEstring( dataStr,  sendTo );  
    return true;
  }

    dataStr="Language pack download (2/2)... \n";
    this->interface->sendBLEstring( dataStr,  sendTo );  

 // Request file from the smart device repo
  filename = country  +".lang";
  if (LittleFS.exists( "/lang/" + country  +".lang") )
    LittleFS.remove( "/lang/" + country  +".lang" );


  httpAddr = "https://github.com/aeonSolutions/AeonLabs-Monitor-Fresh-Reinforced-concrete-Hardening-Strength-maturity/raw/main/translation/" + filename;
  if ( false == this->mWifi->downloadFileHttpGet("/lang/" + country  +".lang",  httpAddr, sendTo) ){
    dataStr="Error Installing the smart device language file \n";
    this->interface->sendBLEstring( dataStr,  sendTo );  
    return true;
  }
 
  dataStr="Download completed. Loading language pack... \n";
  this->interface->sendBLEstring( dataStr,  sendTo );  

  if (this->interface->loadBaseLanguagePack(country, sendTo) && this-> interface->loadDeviceLanguagePack(country, sendTo) ){
    this->interface->config.language = country;
    this->interface->saveSettings();
  }else{
      dataStr="(474) Error loading language pack... \n";
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return true;
  }

  return true;
 }

