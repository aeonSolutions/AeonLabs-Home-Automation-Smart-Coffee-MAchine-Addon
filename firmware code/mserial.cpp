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

#include "mserial.h"
#include <Arduino.h>
#include <semphr.h>
#include "m_file_functions.h"

// **************************** == Serial Class == ************************
mSerial::mSerial(bool DebugMode, HardwareSerial* UARTserial) {
  this->DEBUG_EN = DebugMode;
  this->UARTserial = UARTserial;
  this->MemLockSemaphoreSerial = xSemaphoreCreateMutex();
  this->MemLockSemaphoreUSBSerial = xSemaphoreCreateMutex();

  this->LogFilename="sccd_log.txt";

  this->DEBUG_EN = 1; // ON / OFF
  this->DEBUG_TO = this->DEBUG_BOTH_USB_UART ; // UART, BLE   
  this->DEBUG_TYPE = this->DEBUG_TYPE_VERBOSE; // Verbose // Errors 
  this->DEBUG_SEND_REPOSITORY = 0; // YES/ NO

  this->BLE_IS_DEVICE_CONNECTED = false;
  this->pCharacteristicTX = nullptr;
  
}

// ------------------------------------------------------
void mSerial::setDebugMode(boolean stat){
  this->DEBUG_EN=stat;
}

// ----------------------------------------------------
void mSerial::start(int baud) {
  if (this->DEBUG_EN) {
    if ( this->DEBUG_TO == this->DEBUG_BOTH_USB_UART || this->DEBUG_TO == this->DEBUG_TO_USB ){    
      Serial.begin(115200);
      Serial.println("\nmSerial started on the USB PORT.");
    }

    if ( this->DEBUG_TO == this->DEBUG_BOTH_USB_UART || this->DEBUG_TO == this->DEBUG_TO_UART ){
      this->UARTserial->begin(baud);
      this->UARTserial->println("\nmSerial started on the UART PORT.");
    }
  }
}

// ---------------------------------------------------------
void mSerial::printStrln(String str, uint8_t debugType, uint8_t DEBUG_TO ) {
  this->log(str + String( char(10) ), debugType, DEBUG_TO );
}

// ----------------------------------------------------
void mSerial::printStr(String str, uint8_t debugType, uint8_t DEBUG_TO ) {
  this->log(str, debugType, DEBUG_TO );
}

// ----------------------------------------------------------
  void mSerial::log( String str, uint8_t debugType, uint8_t debugTo ){
    // String mem = "RAM: " + addThousandSeparators( std::string( String(esp_get_free_heap_size() ).c_str() ) )  + " b >> ";
    String mem ="";
    if ( debugTo == this->DEBUG_NONE )
      debugTo = this->DEBUG_TO;

    if ( (this->DEBUG_EN && ( this->DEBUG_TYPE == debugType || this->DEBUG_TYPE == this->DEBUG_TYPE_VERBOSE ) ) || debugType == this->DEBUG_TYPE_INFO ) {

      if ( ( debugTo== this->DEBUG_TO_BLE || debugTo == this->DEBUG_TO_BLE_UART )  ){
          if (this->BLE_IS_DEVICE_CONNECTED)
            this->sendBLEstring(str + String( char(10) ) );
      }

      if ( this->DEBUG_TO == this->DEBUG_TO_UART || this->DEBUG_TO  == this->DEBUG_TO_BLE_UART || this->DEBUG_TO == this->DEBUG_BOTH_USB_UART){ // debug to UART
        if ( debugTo == this->DEBUG_TO_UART || debugTo == this->DEBUG_TO_BLE_UART || debugTo== this->DEBUG_BOTH_USB_UART){ // debug to UART
          xSemaphoreTake(MemLockSemaphoreSerial, portMAX_DELAY); // enter critical section
            this->UARTserial->print(mem);
            this->UARTserial->print(str);
            //this->UARTserial->flush();
          xSemaphoreGive(MemLockSemaphoreSerial); // exit critical section    
        }
      }
      
      if ( this->DEBUG_TO == this->DEBUG_TO_USB || this->DEBUG_TO  == this->DEBUG_TO_BLE_USB || this->DEBUG_TO == this->DEBUG_BOTH_USB_UART){ // debug to UART
        if ( debugTo == this->DEBUG_TO_USB || debugTo == this->DEBUG_TO_BLE_USB  || debugTo == this->DEBUG_BOTH_USB_UART ){ // debug to USB
          xSemaphoreTake(MemLockSemaphoreUSBSerial, portMAX_DELAY); // enter critical section
            Serial.print(mem);
            Serial.print(str);
            //Serial.flush();
          xSemaphoreGive(MemLockSemaphoreUSBSerial); // exit critical section    
        }
      }

      if (this->DEBUG_SEND_REPOSITORY){
          this->saveLog(LittleFS, str);
      }
    }
  }
  
  // -----------------------------------------------------------------
bool mSerial::readSerialData(){
  this->serialDataReceived = "";
  
  if( Serial.available() ){ // if new data is coming from the HW Serial
    while(Serial.available()){
      char inChar = Serial.read();
      if (  (10 != (int)inChar) && (13 != (int)inChar) )
      {
        this->serialDataReceived += String(inChar);
      }
    }
    this->printStr(">");
    return true;
  }else{
    return false;
  }
}

// -----------------------------------------------------------------
bool mSerial::readUARTserialData(){
  this->serialUartDataReceived = "";
  if ( this->UARTserial == nullptr)
    return false;

  if( this->UARTserial->available() ){ // if new data is coming from the HW Serial
    while( this->UARTserial->available() ){
      char inChar = UARTserial->read();
      if (  (10 != (int)inChar) && (13 != (int)inChar) )
      {
        this->serialUartDataReceived += String(inChar);
      }
    }
    this->printStr(">");
    return true;
  }else{
    return false;
  }
}

  // --------------------------------------------------------------------------
bool mSerial::reinitialize_log_file(fs::FS &fs){
    if (fs.exists( "/" +  this->LogFilename ) ){
        if( fs.remove( "/" +  this->LogFilename ) !=true ){
            this->printStrln("Error removing old log file");
            return false;
        }  
    }
}

// --------------------------------------------------------
bool mSerial::saveLog(fs::FS &fs, String str){
  auto logFile = fs.open("/" + this->LogFilename , FILE_WRITE); 
  if (!logFile)
    return false;

  logFile.print(str + String( char(10) ) );
  logFile.close();
  return true;
}

// -----------------------------------------------------
bool mSerial::readLog(fs::FS &fs){
  File logFile = fs.open("/" + this->LogFilename , FILE_READ);
  if (!logFile)
    return false;

  logFile.read();
  logFile.close();
  return true;
}

// -------------------------------------------------------------------------------
void mSerial::sendBLEstring(String message,  uint8_t sendTo){
// no mSerial output allowed here :: reason loop 
//Serial.println("start sendiing BLE message \" " + message+" \" ");
  float remain= message.length() % 20;

  for(int i=0; i<floor(message.length()/20);i++){
    if ( this->pCharacteristicTX != nullptr){
      this->pCharacteristicTX->setValue((message.substring(i*20, (i*20+20) )).c_str());
      this->pCharacteristicTX->notify();
      delay(10);
    }else{
      Serial.print("mserial send BLE msg nullptr");
    }
  }
  
  if (remain > 0.0){
    if ( this->pCharacteristicTX != nullptr){
      this->pCharacteristicTX->setValue((message.substring( floor(message.length()/20)*20, message.length())).c_str());
      this->pCharacteristicTX->notify();  
    }else{
      // ToDo serial
      Serial.print("mserial send BLE msg nullptr");
    }
  }
delay(20);

}
