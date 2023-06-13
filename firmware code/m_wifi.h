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
#include <WiFiMulti.h>
#include "onboard_led.h"
#include <semphr.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "m_file_class.h"

#ifndef INTERFACE_CLASS_DEF
  #include "interface_class.h"
#endif

#ifndef ONBOARD_LED_CLASS_DEF
  #include "onboard_led.h"
#endif
#ifndef MSERIAL_DEF
  #include "mserial.h"
#endif

// OTA updates
#include <esp32FOTA.hpp>

#ifndef M_WIFI_CLASS_DEF
  #define M_WIFI_CLASS_DEF

class M_WIFI_CLASS {
  private:
    WiFiMulti* wifiMulti;
    esp32FOTA* esp32fota;

    long int lastTimeWifiConnectAttempt;
    long int prevTimeErrMsg;
    
    long int            REQUEST_DELTA_TIME_GEOLOCATION;
    long int            $espunixtimePrev;      

   // GBRL commands  *********************************************
    String selected_menu;
    String selected_sub_menu;
    String wifi_ssid;
    String wifi_pwd;

    bool helpCommands(uint8_t sendTo );
    bool wifi_commands(String $BLE_CMD, uint8_t sendTo );
    bool change_device_name(String $BLE_CMD, uint8_t sendTo );

    bool checkErrorMessageTimeLimit();
    bool BLE_IS_DEVICE_CONNECTED; 

  public:
    String OTA_FIRMWARE_SERVER_URL;
    bool ALWAYS_ON_WIFI;

    typedef struct{
      // firmmware update  ***************************
      String firmwareUpdate; // no, manual, auto

      // ********************* WIFI *************************
      bool isWIFIenabled;
      String ssid[5];
      String  password[5];

    // RTC: NTP server ***********************
     String ntpServer;
     long NTP_request_interval;// 64 sec.

    String DEVICE_BLE_NAME;
      
    } config_strut;

    config_strut config;
    long NTP_last_request;
    
    int HTTP_TTL; // 20 sec TTL
    WiFiClientSecure client;

    FILE_CLASS*         drive       = nullptr;
    INTERFACE_CLASS*    interface   = nullptr;
    ONBOARD_LED_CLASS*  onboardLED  = nullptr;
    
    uint32_t          connectionTimeout;
    bool              WIFIconnected;
    uint8_t           number_WIFI_networks;
    SemaphoreHandle_t MemLockSemaphoreWIFI;
    unsigned long $espunixtimeDeviceDisconnected;
    bool forceFirmwareUpdate;

    // Geo Location  ******************************
    String InternetIPaddress;
    String requestGeoLocationDateTime;
    StaticJsonDocument <512> geoLocationInfoJson;


M_WIFI_CLASS();

void init(INTERFACE_CLASS* interface, FILE_CLASS* drive,  ONBOARD_LED_CLASS* onboardLED);
void settings_defaults();

bool start(uint32_t  connectionTimeout, uint8_t numberAttempts);

bool connect2WIFInetowrk(uint8_t numberAttempts);

String get_wifi_status(int status);
void WIFIscanNetworks(bool override = false);

void updateInternetTime();
void resumePowerSavingMode();
void resumeWifiMode();

bool downloadFileHttpGet(String filename, String httpAddr, uint8_t sendTo);
bool get_ip_geo_location_data(String ipAddress = "" , bool override = false);

bool get_ip_address();
uint8_t RSSIToPercent(long rssi);
void startAP();

bool add_wifi_network( String ssid, String password);
void clear_wifi_networks(bool saveSettings= true);
int getNumberWIFIconfigured();
void setNumberWIFIconfigured(uint8_t num);

bool saveSettings(fs::FS &fs = LittleFS);
bool loadSettings(fs::FS &fs = LittleFS);

void init_NTP_Time(char* ntpServer_, long gmtOffset_sec_, int daylightOffset_sec_, long NTP_request_interval_);
bool getBLEconnectivityStatus();
void setBLEconnectivityStatus(bool status);

void startFirmwareUpdate();

   // GBRL commands  *********************************************
bool gbrl_commands(String $BLE_CMD, uint8_t sendTo );
bool firmware(String $BLE_CMD, uint8_t sendTo );


};

// ********************************************
void WIFIevent(WiFiEvent_t event);
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);

#endif