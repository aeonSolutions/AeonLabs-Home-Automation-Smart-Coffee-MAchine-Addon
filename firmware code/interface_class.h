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

#include "Arduino.h"
#include <math.h>
#include  "BLECharacteristic.h"
#include "time.h"
#include "ESP32Time.h"
#include "sha204_i2c.h"
#include "mserial.h"
#include "FS.h"
#include <LittleFS.h>
#include "onboard_led.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <semphr.h>

#ifndef INTERFACE_CLASS_DEF
  #define INTERFACE_CLASS_DEF


  // ******************* Battery Level  *********************
  //* Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);
  #include "BAT_18650_CL.h"

class INTERFACE_CLASS {
  private:
    uint8_t number_WIFI_networks;
  
  public:
    bool LIGHT_SLEEP_EN;

    typedef struct{
      // ******************* Power management **************
      bool isBatteryPowered;
      bool POWER_PLUG_ALWAYS_ON;

      // ********************** onboard sensors *********************
      bool onboard_motion_sensor_en;
      bool onboard_temperature_en;
      bool onboard_humidity;
      double MOTION_SENSITIVITY;

      // RTC ****************************
      long  gmtOffset_sec; // 3600 for 1h difference
      int   daylightOffset_sec;

    // ************* DEBUG *****************
      bool DEBUG_EN; // ON / OFF
      uint8_t DEBUG_TO; // UART, BLE   
      uint8_t DEBUG_TYPE; // Verbose // Errors 
      bool DEBUG_SEND_REPOSITORY; // YES/ NO

      String SENSOR_DATA_FILENAME;

      String DEVICE_PASSWORD;
      String DEVICE_BLE_NAME;

      String language;
      
    } config_strut;

    config_strut config;
// ...................................................
  
  StaticJsonDocument <6000> deviceLangJson;
  StaticJsonDocument <6000> baseLangJson;

  String firmware_version;

    // IO Pin assignment *******************************
    int8_t EXT_PLUG_PWR_EN;
    bool POWER_PLUG_IS_ON;

    int8_t I2C_SDA_IO_PIN;
    int8_t I2C_SCL_IO_PIN;

   // ******************** Battery  **************************
    int8_t BATTERY_ADC_IO;
    Pangodream_18650_CL BL;

    // ******************* MCU frequency  *********************
    //function takes the following frequencies as valid values:
    //  240, 160, 80    <<< For all XTAL types
    //  40, 20, 10      <<< For 40MHz XTAL
    //  26, 13          <<< For 26MHz XTAL
    //  24, 12          <<< For 24MHz XTAL
    // For More Info Visit: https://deepbluembedded.com/esp32-change-cpu-speed-clock-frequency/

    int8_t SAMPLING_FREQUENCY ; 

    int MAX_FREQUENCY ; 
    int WIFI_FREQUENCY ; // min WIFI MCU Freq is 80-240
    int MIN_MCU_FREQUENCY ;
    int SERIAL_DEFAULT_SPEED;    
    int MCU_FREQUENCY_SERIAL_SPEED;
    int CURRENT_CLOCK_FREQUENCY;
    uint32_t Freq = 0;
    
    SemaphoreHandle_t McuFreqSemaphore = xSemaphoreCreateMutex();
    bool McuFrequencyBusy;
    
    // ************************** == Lock semaphore == **********************
  SemaphoreHandle_t MemLockSemaphoreCore1 = xSemaphoreCreateMutex();
  SemaphoreHandle_t MemLockSemaphoreCore2 = xSemaphoreCreateMutex();

   // *********************** BLE **************************
     BLECharacteristic *pCharacteristicTX;  

   // Task scheduler *********************************
    unsigned long $espunixtime;
    unsigned long $espunixtimePrev;
    unsigned long $espunixtimeStartMeasure;
    String measurement_Start_Time;

       // RTC SETUP *******************
    ESP32Time rtc;  // offset in seconds GMT
    struct tm timeinfo;


    // Sensors ****************************************
    int8_t NUMBER_OF_SENSORS; 
    bool Measurments_EN; // start / end collecting data
    bool Measurments_NEW; // start / end collecting data

    // UNIQUE FingerPrint ID for Live data Authenticity and Authentication ******************************
    atsha204Class sha204 = atsha204Class();
    /* WARNING: is required to set BUFFER_LENGTH to at least 64 in Wire.h and twi.h  */
    const char *DATA_VALIDATION_KEY;

    // functions and methods  ****************************
    INTERFACE_CLASS();

    mSerial* mserial;
    ONBOARD_LED_CLASS* onBoardLED;
    HardwareSerial* UARTserial;

    void init(mSerial* mserial, bool DEBUG_ENABLE);
    void init_BLE(BLECharacteristic *pCharacteristicTX);
    void settings_defaults();

    void setBLEconnectivityStatus(bool status);
    bool getBLEconnectivityStatus();

    bool loadSettings( fs::FS &fs = LittleFS );
    bool saveSettings( fs::FS &fs = LittleFS );

    void sendBLEstring(String message="",  uint8_t sendTo = mSerial::DEBUG_ALL_USB_UART_BLE );

    void init_NTP_Time(char* ntpServer_="pool.ntp.org", long gmtOffset_sec_=0, int daylightOffset_sec=3600, long NTP_request_interval_=64000);

    bool setMCUclockFrequency(int clockFreq);

    bool loadDeviceLanguagePack(String country, uint8_t sendTo );
    bool loadBaseLanguagePack(String country, uint8_t sendTo );
    bool loadDefaultLanguagePack(uint8_t sendTo = mSerial::DEBUG_BOTH_USB_UART );

    String DeviceTranslation(String key);
    String BaseTranslation(String key);
};

#endif