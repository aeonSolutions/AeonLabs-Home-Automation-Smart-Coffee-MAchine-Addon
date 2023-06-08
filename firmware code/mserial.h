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

#include <Arduino.h>
#include <semphr.h>
#include "FS.h"
#include <LittleFS.h>
#include  "BLECharacteristic.h"
#include <HardwareSerial.h>
#include "USB.h"


#ifndef  MSERIAL_DEF
  #define  MSERIAL_DEF


  class mSerial {
    private:
      SemaphoreHandle_t MemLockSemaphoreSerial = xSemaphoreCreateMutex();
      SemaphoreHandle_t MemLockSemaphoreUSBSerial = xSemaphoreCreateMutex();
      
      void log(String str, uint8_t debugType, uint8_t sendTo );
      HardwareSerial* UARTserial;

    public:
       // *********************** BLE **************************
     BLECharacteristic *pCharacteristicTX;   
      bool BLE_IS_DEVICE_CONNECTED;
      
      // ************* DEBUG *****************
      static constexpr uint8_t DEBUG_TYPE_VERBOSE = 100 ;
      static constexpr uint8_t DEBUG_TYPE_ERRORS = 101;
      static constexpr uint8_t DEBUG_TYPE_INFO = 102;

      static constexpr uint8_t DEBUG_NONE = -1;

      static constexpr uint8_t DEBUG_TO_BLE = 10;
      static constexpr uint8_t DEBUG_TO_BLE_UART = 12;
      static constexpr uint8_t DEBUG_TO_BLE_USB = 15;

      static constexpr uint8_t DEBUG_TO_UART = 11;
      static constexpr uint8_t DEBUG_TO_USB = 13;
      static constexpr uint8_t DEBUG_BOTH_USB_UART = 14;
      static constexpr uint8_t DEBUG_ALL_USB_UART_BLE = 14;

      bool DEBUG_EN; // ON / OFF
      uint8_t DEBUG_TO; // UART, BLE, USB   
      uint8_t DEBUG_TYPE; // Verbose // Errors 
      bool DEBUG_SEND_REPOSITORY; // YES/ NO
      
      String LogFilename;
      String serialDataReceived;
      String serialUartDataReceived;

      bool reinitialize_log_file(fs::FS &fs);
      void setDebugMode(boolean stat);

      bool saveLog(fs::FS &fs, String str);
      bool readLog(fs::FS &fs);


// ******************************
      mSerial(bool DebugMode, HardwareSerial* UARTserial);
      void start(int baud);

      void printStrln( String str,   uint8_t debugType = mSerial::DEBUG_TYPE_INFO , uint8_t DEBUG_TO = mSerial::DEBUG_NONE ); // default DEBUG_TYPE_VERBOSE
      void printStr( String str,  uint8_t debugType = mSerial::DEBUG_TYPE_INFO, uint8_t DEBUG_TO = mSerial::DEBUG_NONE  );
      
      void sendBLEstring(String message, uint8_t sendTo = mSerial::DEBUG_TO_BLE);
      bool readSerialData();
      bool readUARTserialData();

  };
#endif