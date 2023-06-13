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

#include "m_wifi.h"
#include "time.h"
#include "ESP32Time.h"
#include "HTTPClient.h"
#include "Config.h"
#include "m_file_functions.h"
#include "github_cert.h"
#include "esp_wifi.h"

#ifndef ESP32PING_H
  #include <ESP32Ping.h>
#endif

M_WIFI_CLASS::M_WIFI_CLASS(){
  this->MemLockSemaphoreWIFI = xSemaphoreCreateMutex();
  this->WIFIconnected=false;
  this->BLE_IS_DEVICE_CONNECTED=false;
}


// **********************************************
void M_WIFI_CLASS::init(INTERFACE_CLASS* interface, FILE_CLASS* drive, ONBOARD_LED_CLASS* onboardLED){
    this->drive = drive;
    this->interface=interface;
    this->interface->mserial->printStr("init wifi ...");
    this->onboardLED=onboardLED;
    
    this->REQUEST_DELTA_TIME_GEOLOCATION  = 10*60*1000; // 10 min 
    this->$espunixtimePrev= millis(); 
    
    this->number_WIFI_networks=0;
    this-> prevTimeErrMsg= millis() + 120000;

    this->HTTP_TTL= 20000; // 20 sec TTL
    this->lastTimeWifiConnectAttempt=millis();
    this->wifiMulti= new WiFiMulti();
    this->prevTimeErrMsg = millis() - 60000;
    
    WiFi.onEvent(WIFIevent);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    configTime(this->interface->config.gmtOffset_sec, this->interface->config.daylightOffset_sec, this->config.ntpServer.c_str());
    
    this->BLE_IS_DEVICE_CONNECTED=false;
    this->interface->mserial->printStrln("done");
}
// **************************************************

void M_WIFI_CLASS::settings_defaults(){
  this->config.firmwareUpdate="auto";
  this->config.isWIFIenabled=false;
   
  this->BLE_IS_DEVICE_CONNECTED=false;
  this->config.DEVICE_BLE_NAME="AeonHome 12A";
  this->forceFirmwareUpdate=false;
  
  this->clear_wifi_networks(false);
  
   this->interface->mserial->printStrln("wifi settings defaults loaded.");
}

// ************************************************

bool M_WIFI_CLASS::start(uint32_t  connectionTimeout, uint8_t numberAttempts){
  if (WiFi.status() == WL_CONNECTED)
    return true;

  this->connectionTimeout=connectionTimeout;
  
  if (this->getNumberWIFIconfigured() == 0 ){
    this->lastTimeWifiConnectAttempt=millis();
    if ( this->checkErrorMessageTimeLimit() ){
      this->interface->mserial->printStrln("WIFI: You need to add a wifi network first", this->interface->mserial->DEBUG_TYPE_ERRORS);
      this->interface->onBoardLED->led[0] = this->interface->onBoardLED->LED_RED;
      this->interface->onBoardLED->statusLED(100, 1);
    }
    
    // this->startAP(); web server
    return false;
  }

  if ( this->interface->CURRENT_CLOCK_FREQUENCY < this->interface->WIFI_FREQUENCY )
    this->resumeWifiMode();
  delay(500);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  delay(500);

  for(uint8_t i=0; i < 5; i++){
    if (this->config.ssid[i] !="")
      this->wifiMulti->addAP(this->config.ssid[i].c_str(), this->config.password[i].c_str());        
  }

  interface->onBoardLED->led[0] = interface->onBoardLED->LED_BLUE;
  interface->onBoardLED->statusLED(100, 1);

  this->connect2WIFInetowrk(numberAttempts);
  this->lastTimeWifiConnectAttempt=millis();
  return true;
}


// ********************************************

bool M_WIFI_CLASS::checkErrorMessageTimeLimit(){
  if( abs( long(millis() - this-> prevTimeErrMsg) ) > 60000 ){
    this-> prevTimeErrMsg = millis();
    return false;
  }
    return true;
}
// **********************************************************

void M_WIFI_CLASS::startAP() {
  this->interface->mserial->printStrln("Starting access point...");
  IPAddress apIP(192, 168, 1, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
}
// **************************************************

bool M_WIFI_CLASS::connect2WIFInetowrk(uint8_t numberAttempts){
  if (WiFi.status() == WL_CONNECTED)
    return true;
  
  if (this->getNumberWIFIconfigured() == 0 ){
    if ( this->checkErrorMessageTimeLimit() ){
      this->interface->mserial->printStrln("C2W: You need to add a wifi network first", this->interface->mserial->DEBUG_TYPE_ERRORS);
    }
    this->lastTimeWifiConnectAttempt=millis();
    return false;
  }
  
 
  int WiFi_prev_state=-10;
  int cnt = 0;        
  uint8_t statusWIFI=WL_DISCONNECTED;
  
  this->interface->mserial->printStr("Connecting ( "+ String(cnt+1) + ") : ");
  while (statusWIFI != WL_CONNECTED) {
    // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
    this->interface->mserial->printStr( "# " );
    if(this->wifiMulti->run(this->connectionTimeout) == WL_CONNECTED) {        
      this->interface->mserial->printStrln( "Connection Details");
      this->interface->mserial->printStrln( "   Network : " + String( WiFi.SSID() ) + " (" + String( RSSIToPercent(WiFi.RSSI() ) ) + ")" );
      this->interface->mserial->printStrln( "        IP : "+WiFi.localIP().toString());
      this->interface->mserial->printStrln( "   Gateway : "+WiFi.gatewayIP().toString());

      if(!Ping.ping("www.google.com", 3)){
        this->interface->mserial->printStrln( "no Internet connectivity found.");
      }else{
        //init time
        configTime(this->interface->config.gmtOffset_sec, this->interface->config.daylightOffset_sec, this->config.ntpServer.c_str());
        this->updateInternetTime();
        this->get_ip_geo_location_data();
      }
      return true;
    }
    
    cnt++;
    if (cnt == numberAttempts ){ 
        this->interface->mserial->printStr( "." );
        this->interface->mserial->printStrln("\nCould not connect to a WIFI network after " + String(numberAttempts) + " attempts\n");
        this->WIFIconnected=false;
        return false;       
    }
  }
  
  return true;
}
// ****************************************************

void M_WIFI_CLASS::clear_wifi_networks(bool saveSettings){
  for(int i=0; i<5 ; i++){
    this->config.ssid[i] = "";
    this->config.password[i] = "";
  }
  this->number_WIFI_networks=0;
  if (saveSettings)
    this->saveSettings();
}
// ****************************************************

bool M_WIFI_CLASS::add_wifi_network(String  ssid, String password){
  if (this->config.ssid[0] == NULL)
    this->clear_wifi_networks();

  for(int i=0; i<5 ; i++){
    if (this->config.ssid[i] == ssid){
      this->interface->mserial->printStrln("WIFI network already on the list");
      return false;
    }
  }

  this->config.ssid[this->number_WIFI_networks]=ssid;
  this->config.password[ this->number_WIFI_networks]= password;
  this->number_WIFI_networks++;

  if( this->number_WIFI_networks>4)
    this->number_WIFI_networks=0;
    
  this->saveSettings();
  return true;
}
// ***************************************************

int M_WIFI_CLASS::getNumberWIFIconfigured(){  
  return  this->number_WIFI_networks;
}

void M_WIFI_CLASS::setNumberWIFIconfigured(uint8_t num){  
  this->number_WIFI_networks= num;
}
// ********************************************************
 
 void M_WIFI_CLASS::resumePowerSavingMode(){
    this->interface->mserial->printStrln("WIFI: setting power saving mode.");
    if (this->ALWAYS_ON_WIFI == false){
      WiFi.disconnect(true);
      delay(100);
      WiFi.mode(WIFI_MODE_NULL);
      this->interface->setMCUclockFrequency( interface->MIN_MCU_FREQUENCY);
      interface->CURRENT_CLOCK_FREQUENCY = interface->MIN_MCU_FREQUENCY;
    }else{
      this->interface->setMCUclockFrequency( interface->WIFI_FREQUENCY);
      interface->CURRENT_CLOCK_FREQUENCY = interface->WIFI_FREQUENCY;
    }

    this->$espunixtimeDeviceDisconnected = millis();
 }

// ********************************************************
 void M_WIFI_CLASS::resumeWifiMode(){
    xSemaphoreTake(this->interface->McuFreqSemaphore, portMAX_DELAY);
      this->interface->McuFrequencyBusy = true;

      this->interface->setMCUclockFrequency( interface->WIFI_FREQUENCY);
      interface->CURRENT_CLOCK_FREQUENCY = interface->WIFI_FREQUENCY;

      this->interface->mserial->printStrln("setting to WIFI EN CPU Freq = " + String(getCpuFrequencyMhz()));
      this->interface->McuFrequencyBusy = false;
  xSemaphoreGive(this->interface->McuFreqSemaphore);
 }
// ********************************************************

String M_WIFI_CLASS::get_wifi_status(int status){
    switch(status){
        case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS(0): WiFi is in process of changing between statuses";
        case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED(2): Successful connection is established";
        case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL(1): SSID cannot be reached";
        case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED (4): Password is incorrect";
        case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST (5)";
        case WL_CONNECTED:
        return "WL_CONNECTED (3)";
        case WL_DISCONNECTED:
        return "WL_DISCONNECTED (6): Module is not configured in station mode";
    }
}

// *********************************************************
void M_WIFI_CLASS::WIFIscanNetworks(bool override){

  if ( WiFi.status() == WL_CONNECTED && override == false )
    return;
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  
  String dataStr = "";

  if (n == 0) {
    this->interface->sendBLEstring( "no nearby WIFI networks found\n", mSerial::DEBUG_ALL_USB_UART_BLE);
  } else {
    dataStr = "\n" + String(n) + " WiFi networks nearby:\n";
    this->interface->sendBLEstring(dataStr , mSerial::DEBUG_ALL_USB_UART_BLE);
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      dataStr = String(i + 1) + ": " + String(WiFi.SSID(i)) + " (" + String(WiFi.RSSI(i)) + ")";
      dataStr += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
      this->interface->sendBLEstring(dataStr, mSerial::DEBUG_ALL_USB_UART_BLE);
      delay(10);
    }
  }  
}

// ****************************************************

void M_WIFI_CLASS::setBLEconnectivityStatus(bool status){
  this->interface->mserial->printStrln("BLE status is " +String(status));

  this->BLE_IS_DEVICE_CONNECTED = status;
  this->interface->mserial->BLE_IS_DEVICE_CONNECTED = status;
  if (status == false)
    this->$espunixtimeDeviceDisconnected = millis();
}
// ........................................
bool M_WIFI_CLASS::getBLEconnectivityStatus(){
  return this->BLE_IS_DEVICE_CONNECTED;
}


// ****************************************************

void M_WIFI_CLASS::init_NTP_Time(char* ntpServer_, long gmtOffset_sec_, int daylightOffset_sec_, long NTP_request_interval_){
  this->config.ntpServer = String(ntpServer_);
  this->interface->config.gmtOffset_sec = gmtOffset_sec_; // 3600 for 1h difference
  this->interface->config.daylightOffset_sec = daylightOffset_sec_;
  this->config.NTP_request_interval=NTP_request_interval_;// 64 sec.
  this->NTP_last_request=0;
  this->interface->rtc =  ESP32Time(this->interface->config.gmtOffset_sec);
  configTime(this->interface->config.gmtOffset_sec, this->interface->config.daylightOffset_sec, this->config.ntpServer.c_str());

  this->interface->mserial->printStrln("set RTC clock to firmware Date & Time ...");  
  this->interface->rtc.setTimeStruct(CompileDateTime(__DATE__, __TIME__)); 

  this->interface->mserial->printStrln(this->interface->rtc.getDateTime(true));

}

// ********************************************************************************

bool M_WIFI_CLASS::saveSettings(fs::FS &fs){

  
  this->interface->mserial->printStrln("Saving the Smart Device WIFI settings...");

  if (fs.exists("/wifi_settings.cfg") )
    fs.remove("/wifi_settings.cfg");

  File settingsFile = fs.open("/wifi_settings.cfg", FILE_WRITE); 
  if ( !settingsFile ){
    Serial.println("error creating wifi_settings file.");
    settingsFile.close();
    return false;
  }

  settingsFile.print( this->config.firmwareUpdate + String(';'));
  settingsFile.print( String(this->config.isWIFIenabled) + String(';') );

  String ssid="";
  String pwd="";
  for(int i=0; i<5 ; i++){
    settingsFile.print( this->config.ssid[i] + String(';'));
    settingsFile.print( this->config.password[i] + String(';'));
  }

  settingsFile.print( this->config.ntpServer + String(';'));

  settingsFile.print( String(this->config.NTP_request_interval) + String(';'));

  settingsFile.print( this->config.DEVICE_BLE_NAME + String(';'));

  settingsFile.close();
  return true;
}

// ********************************************************************************
bool M_WIFI_CLASS::loadSettings(fs::FS &fs){
  this->interface->mserial->printStr("Loading the Smart Device WIFI settings...");
  
  File settingsFile = fs.open("/wifi_settings.cfg", FILE_READ);
  if (!settingsFile){
    this->interface->mserial->printStrln("not found.");
    settingsFile.close();
    return false;
  }
  if (settingsFile.size() == 0){
    this->interface->mserial->printStrln("Invalid file");
    settingsFile.close();
    return false;    
  }

  File settingsFile2 = fs.open("/wifi_settings.cfg", FILE_READ);
 // Serial.println("full contents:");
 // Serial.println( settingsFile2.readString() );
 // Serial.println(" === END ===== ");
        
    this->interface->mserial->printStrln("done. Size: " + String( settingsFile.size()) );

          // firmmware update  ***************************
    this->config.firmwareUpdate = settingsFile.readStringUntil(';');
      // ********************* WIFI *************************
    String temp= settingsFile.readStringUntil(';');
    this->config.isWIFIenabled = *(temp.c_str()) != '0';

    this->setNumberWIFIconfigured(0);
    for(int i=0; i<5; i++){
      temp = settingsFile.readStringUntil(';');
      temp.trim();
      this->config.ssid[i] = temp;

      temp = settingsFile.readStringUntil(';');
      temp.trim();
      this->config.password[i] = temp;
      
      if (this->config.ssid[i].length()>0 && isStringAllSpaces(this->config.ssid[i])==false ){
        this->setNumberWIFIconfigured(this->getNumberWIFIconfigured()+1);
      }
    }
    this->interface->mserial->printStrln( "number of wifi networks is " + String(this->getNumberWIFIconfigured()));

    // RTC: NTP server ***********************
    this->config. ntpServer= settingsFile.readStringUntil(';');    
    this->config.NTP_request_interval = atol(settingsFile.readStringUntil( ';' ).c_str() ); 

    this->config.DEVICE_BLE_NAME = settingsFile.readStringUntil(';');
  
  settingsFile.close();

  return true;
}

// *************************************************************
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{

}

void WIFIevent(WiFiEvent_t event){
  switch (event) {
    case SYSTEM_EVENT_WIFI_READY: 
      Serial.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("Connected to access point");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi access point");
      //WiFi.begin(ssid, password);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("Connection has Internet connectivity.");
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
     Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
      Serial.println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
     Serial.println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
     Serial.println("Received probe request");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      Serial.println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
     Serial.println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
    default: break;
  }
}


// *************************************************************
void M_WIFI_CLASS::updateInternetTime(){  
  if (WiFi.status() != WL_CONNECTED)
    return;

  long diff= (millis()- this->NTP_last_request);
  if(abs(diff)< this->config.NTP_request_interval && this->NTP_last_request!=0){
    this->interface->mserial->printStrln("Internet Time (NTP) is up to date. ");
    return;
  }
  this->NTP_last_request=millis();

  this->interface->mserial->printStrln("Requesting Internet Time (NTP) to "+ String(this->config.ntpServer));
  if(!getLocalTime(&this->interface->timeinfo)){
    this->interface->mserial->printStrln("Failed to obtain Internet Time. Current System Time is " + String(this->interface->rtc.getDateTime(true)) , this->interface->mserial->DEBUG_TYPE_ERRORS);
    return;
  }else{
    this->interface->mserial->printStr("Internet Time is ");
    Serial.println(&this->interface->timeinfo, "%A, %B %d %Y %H:%M:%S");

    this->interface->rtc.setTimeStruct(this->interface->timeinfo); 

    //this->interface->rtc.setTime(this->interface->timeinfo.tm_hour, this->interface->timeinfo.tm_min, this->interface->timeinfo.tm_sec,
    //                              this->interface->timeinfo.tm_mday, this->interface->timeinfo.tm_mon,this->interface->timeinfo.tm_year); 

    this->interface->mserial->printStrln("Local Time is: " + String(this->interface->rtc.getDateTime(true)));
  }
}

// ******************************************************************

bool M_WIFI_CLASS::downloadFileHttpGet(String filename, String httpAddr, uint8_t sendTo){
  String dataStr = "";

  if ( LittleFS.exists(filename) ){
    dataStr = "DW WIFI: File requested already exists. ";
    this->interface->sendBLEstring( dataStr,  sendTo );  
    return false;
  }

 if (WiFi.status() != WL_CONNECTED){
    this->start(10000, 5); // TTL , n attempts 
  }
  
  if (WiFi.status() != WL_CONNECTED ){

      this->interface->mserial->printStrln("WIFI DW GET: unable to connect to WIFI.");
      this->interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
      this->interface->onBoardLED->statusLED(100, 1);
    return false;
  }
  
  File DW_File = LittleFS.open(filename, FILE_WRITE);
  if ( !DW_File ){
      this->interface->mserial->printStrln("WIFI DW GET: error creating  file");
      this->interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
      this->interface->onBoardLED->statusLED(100, 1);
    return false;
  }
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    // configure server and url
    http.begin(httpAddr);
    // start connection and send HTTP header
    int httpCode = http.GET();
    if(httpCode < 1) {
      dataStr = "HTTP GET failed with error code " + String(http.errorToString(httpCode).c_str()) + " \n";
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return false;
    }

    if(httpCode != HTTP_CODE_OK) {
      dataStr = "Server returned " + String(httpCode) + " error code\n";
      this->interface->sendBLEstring( dataStr,  sendTo );  
      return false;
    }

    // get length of document (is -1 when Server sends no Content-Length header)
    int len = http.getSize();

    // create buffer for read
    uint8_t buff[128] = { 0 };

    // get tcp stream
    WiFiClient * stream = http.getStreamPtr();

    // read all data from server
    while(http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();

        if(size) {
            // read up to 128 byte
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

            DW_File.write(buff,c);
            if(len > 0) {
                len -= c;
            }
        }
        delay(1);
  }

  DW_File.close();

  http.end();
  return true;

}
// ****************************************************************
// *************************************************************************
bool M_WIFI_CLASS::get_ip_address(){
  if(WiFi.status() != WL_CONNECTED)
    return false;

  HTTPClient http;

  String serverPath = "http://api.ipify.org";
  this->interface->mserial->printStr("request external IP address...");
  http.begin(serverPath.c_str());      
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  if (httpResponseCode>0) {
    this->interface->mserial->printStrln("done (" + String(httpResponseCode) + ")." );
    if (httpResponseCode == 200){
      this->InternetIPaddress = http.getString();
      http.end();
      return true;
    }else{
      this->interface->mserial->printStrln("Error retrieving External IP address");
    }
  }else {
    this->interface->mserial->printStrln("response code error: " + String(httpResponseCode) );
  }
  // Free resources
  http.end();
  return false;
}

// *************************************************************************
bool M_WIFI_CLASS::get_ip_geo_location_data(String ipAddress , bool override ){

  if ( override==false){
    if ( ( millis() - this->$espunixtimePrev) < this->REQUEST_DELTA_TIME_GEOLOCATION )
      return false;
  }

  if (WiFi.status() != WL_CONNECTED ){
    this->interface->mserial->printStrln("GEO Location: unable to connect to WIFI.");
    this->interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
    this->interface->onBoardLED->statusLED(100, 1);
    return false;
  }

  this->$espunixtimePrev= millis();
  
  HTTPClient http;
  String serverPath = "http://ip-api.com/json/";
  if (ipAddress==""){
    if (this->InternetIPaddress==""){
      if ( false == this->get_ip_address() )
        return false;
    }
    serverPath += this->InternetIPaddress;
  }else{
      serverPath += ipAddress;
  }

  this->interface->mserial->printStrln( "Requesting Geo Location data... one moment." ); 
  
  http.begin(serverPath.c_str());      
  // Send HTTP GET request
  int httpResponseCode = http.GET();
      
  if (httpResponseCode<1 && httpResponseCode != 200 ) {
      this->interface->mserial->printStrln("Http error " + String(httpResponseCode) );
    return false;
  }
  
  String JSONpayload = http.getString();
     
  // Parse JSON object
        /*
      {
          "status":"success",
          "country":"Belgium",
          "countryCode":"BE",
          "region":"BRU",
          "regionName":"Brussels Capital",
          "city":"Brussels",
          "zip":"1000",
          "lat":50.8534,
          "lon":4.347,
          "timezone":"Europe/Brussels",
          "isp":"PROXIMUS",
          "org":"",
          "as":"AS5432 Proximus NV",
          "query":"37.62.11.2"
      }
      String stat = this->datasetInfoJson["status"];
      */

  DeserializationError error = deserializeJson(this->geoLocationInfoJson, JSONpayload);
  if (error) {
      this->interface->mserial->printStrln("Error deserializing JSON");
      
      interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
      interface->onBoardLED->statusLED(100, 1);
      return false;
  }else{
    this->requestGeoLocationDateTime= String( this->interface->rtc.getDateTime(true) );
  }
  // Free resources
  http.end();

  interface->onBoardLED->led[0] = interface->onBoardLED->LED_GREEN;
  interface->onBoardLED->statusLED(100, 1);
  return true;
}
// *************************************************

uint8_t M_WIFI_CLASS::RSSIToPercent(long rssi) {
  if (rssi >= -50) return 100;
  else if (rssi <= -100) return 0;
  else return (rssi + 100) * 2;
  // ▂▄▆█
}
// ********************************************************

// Over The Air Firmware Update ********************************************************
// ************************************************************************************
void M_WIFI_CLASS::startFirmwareUpdate(){
    if (this->start(10000, 5) == false)
      return; 

    this->forceFirmwareUpdate=false;
    
  if ( this->getBLEconnectivityStatus()){
    if ( this->geoLocationInfoJson.containsKey("country") && this->geoLocationInfoJson.containsKey("regionName") ){
      this->interface->mserial->printStrln("done. \n Country:" + String(this->geoLocationInfoJson["country"].as<char*>()) + "\n Region: " + String(this->geoLocationInfoJson["regionName"].as<char*>()) + "\n");
      this->interface->sendBLEstring("done. \n Country:" + String(this->geoLocationInfoJson["country"].as<char*>()) + "\n Region: " + String(this->geoLocationInfoJson["regionName"].as<char*>()) + "\n");
      delay(500);
    }
  }

  
  if ( this->getBLEconnectivityStatus()){
    delay(500);
    this->interface->sendBLEstring("done. \nRequesting the lastest firmware revision....");
    delay(500);
  }

  this->esp32fota = new esp32FOTA("esp32-fota-http", "0.0.0", false);
  //init OTA updates
  {
    auto cfg = this->esp32fota->getConfig();
    cfg.name          = "esp32-fota-http";
    cfg.manifest_url  = (char*) this->OTA_FIRMWARE_SERVER_URL.c_str();
    cfg.sem           = SemverClass( this->interface->firmware_version.substring(0, this->interface->firmware_version.indexOf(".")).toInt() , this->interface->firmware_version.substring( this->interface->firmware_version.indexOf(".")+1, this->interface->firmware_version.lastIndexOf(".")).toInt(), this->interface->firmware_version.substring( this->interface->firmware_version.lastIndexOf(".")+1, this->interface->firmware_version.length() ).toInt() ); // major, minor, patch
    cfg.check_sig     = false; // verify signed firmware with rsa public key
    cfg.unsafe        = true; // disable certificate check when using TLS
    cfg.root_ca       = new CryptoMemAsset("Root CA", GITHUB_ROOT_CA_RSA_SHA384, strlen(GITHUB_ROOT_CA_RSA_SHA384)+1 );
    //cfg.pub_key       = MyRSAKey;
    //cfg.use_device_id = false;
    this->esp32fota->setConfig( cfg );
  }

  //esp32FOTA.setManifestURL( manifest_url );
  bool updatedNeeded = this->esp32fota->execHTTPcheck();
  if (updatedNeeded) {
    this->interface->onBoardLED->led[0] = this->interface->onBoardLED->LED_BLUE;
    this->interface->onBoardLED->statusLED(100,0); 
    this->interface->mserial->printStrln("new firmware version found. Starting update ...");
    if ( this->getBLEconnectivityStatus()){
      delay(500);
      this->interface->sendBLEstring("done. \nNew firmware version found. Starting to download and upodate. The Device will reboot when completed. ");
      delay(500);
    }

    this->esp32fota->execOTA();
    if (this->ALWAYS_ON_WIFI == false)
      WiFi.disconnect(true);
    return;
  }else{
    this->interface->mserial->printStrln("no firmware update needed.");
    if ( this->getBLEconnectivityStatus()){
      delay(500);
      this->interface->sendBLEstring("done. \nNo new firmware available");
      delay(500);
    }
    this->interface->onBoardLED->led[0] = this->interface->onBoardLED->LED_GREEN;
    this->interface->onBoardLED->statusLED(100, 1);   
  }
  delete this->esp32fota;
  this->esp32fota = nullptr;
  if (this->ALWAYS_ON_WIFI == false)
    WiFi.disconnect(true);
}


   // GBRL commands  *********************************************
   // ***********************************************************
bool M_WIFI_CLASS::gbrl_commands(String $BLE_CMD, uint8_t sendTo ){
  String dataStr="";

  if($BLE_CMD=="$?" || $BLE_CMD=="$help"){
    return  this->helpCommands( sendTo );
  }else if($BLE_CMD=="$view dn" ){
    dataStr =  "The devie BLE name is " + this->interface->config.DEVICE_BLE_NAME + "\n";
    this->interface->sendBLEstring(dataStr,  sendTo ); 
    return true; 
    
  }else if($BLE_CMD.indexOf("$set dn ")>-1){
      return this->change_device_name($BLE_CMD,   sendTo );
  
  }else if($BLE_CMD.indexOf("$wifi ")>-1 || this->selected_menu=="$wifi pwd" || this->selected_menu=="$wifi ssid"){
    return this->wifi_commands( $BLE_CMD,   sendTo );
  }

  if($BLE_CMD=="$geo info"){
    this->start(10000,5);
    if ( false == this->get_ip_geo_location_data( "", true ) ){
      return true;
    }

    dataStr="GeoLocation Data:\n";
    dataStr += "Internet I.P. address: " + this->InternetIPaddress + "\n";
    dataStr += "Time of last request: " + this->requestGeoLocationDateTime + "\n";
    
    if ( this->geoLocationInfoJson.isNull() == true ){
      dataStr="NULL geoLocation data.\n";
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;
    }

    if(this->geoLocationInfoJson.containsKey("lat")){
      float lat = this->geoLocationInfoJson["lat"];
      dataStr += "Latitude: "+ String(lat,4) + "\n";
    }
    if(this->geoLocationInfoJson.containsKey("lon")){
      float lon = this->geoLocationInfoJson["lon"];
      dataStr += "Longitude: "+ String(lon,4) + "\n";
    }

    if(this->geoLocationInfoJson.containsKey("regionName"))
      dataStr += String(this->geoLocationInfoJson["regionName"].as<char*>()) + ", ";       
    
    if(this->geoLocationInfoJson.containsKey("country"))
      dataStr += String(this->geoLocationInfoJson["country"].as<char*>()); 
    
    if(this->geoLocationInfoJson.containsKey("countryCode"))
      dataStr += "(" + String(this->geoLocationInfoJson["countryCode"].as<char*>()) + ")\n\n"; 

    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true;
  }
  
  return false;
}

// *********************************************************
 bool M_WIFI_CLASS::helpCommands(uint8_t sendTo ){
    String dataStr="Device Connectivity Commands:\n" \

                    "$set dn [name]                     - Set device BLE name (max 20 chars)\n" \
                    "$view dn                           - View device BLE name\n" \
                    "\n" \
                    "$wifi status                       - View WIFI status\n" \
                    "$wifi nearby                       - View WIFI networks nearby\n" \
                    "$wifi networks                     - View configured WIFI networks\n" \
                    "$wifi ssid                         - Add WIFI network\n" \
                    "$wifi clear                        - Clear all WIFI credentials\n" \
                    "$wifi default                      - Enable \"SCC WIFI\" default SSID Network\n" \
                    "\n" \
                    "$geo info                          - View Geo-location data information (requires a WIFI connection)\n\n";

    this->interface->sendBLEstring( dataStr,  sendTo ); 
    
    return false; 
 }

 // *********************************************************
bool M_WIFI_CLASS::wifi_commands(String $BLE_CMD, uint8_t sendTo ){
  String dataStr="";
  
  if($BLE_CMD.indexOf("$firmware cfg ")>-1){
      return this->firmware($BLE_CMD,  sendTo );
  }

  if($BLE_CMD=="$firmware update"){
    this->forceFirmwareUpdate=true;
    dataStr = "Starting Firmware update... one moment.\n";
    this->interface->sendBLEstring( dataStr,  sendTo );  
    return true;
  }

  if($BLE_CMD=="$wifi default"){
    this->clear_wifi_networks();
    this->add_wifi_network("SCC WIFI", "1234567890");
    dataStr = "Default WIFI network is SET.\n";
    dataStr = "You need to setup your WIFI Access Point with the follwoing credentials:\n";
    dataStr += "Network SSID: \"SCC WIFI\"\n";
    dataStr += "Password: \"1234567890\"\n\n";
    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true;
  }  

  if($BLE_CMD=="$wifi nearby"){
      this->WIFIscanNetworks(true);
      return true;
  }

  if($BLE_CMD=="$wifi status"){
      dataStr= "Current WIFI status:" + String( char(10));
      dataStr += "     IP     : " + WiFi.localIP().toString() + "\n";
      dataStr +=  "    Gateway: " + WiFi.gatewayIP().toString() + "\n\n";
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;
  }
  
  if($BLE_CMD=="$wifi clear"){
      dataStr= "All WIFI credentials were deleted." + String( char(10));
      this->clear_wifi_networks();
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;
  }
// .....................................................................................
  if (this->selected_menu=="$wifi pwd"){   
    this->selected_menu = "wifi pwd completed";
    String ssid = this->wifi_ssid;
    String pwd;
    String mask="";
    if ( $BLE_CMD == "none"){
      pwd = "";
      mask= "no passowrd";
    }else{
      pwd = $BLE_CMD;
      mask = "**********";
    }
    if ( this->add_wifi_network(  ssid, pwd ) == false ){
      return true;
    }
        
    dataStr= mask + "\n";
  }

  if (this->selected_menu=="wifi pwd completed" || $BLE_CMD=="$wifi networks"){
    this->selected_menu = ""; 
    dataStr += "WIFI Netwrok List: " + String( char(10));

    for(int i=0; i<5 ; i++){
      if ( this->config.ssid[i] != ""){
        dataStr += this->config.ssid[i];
        if ( this->config.password[i] != ""){
          dataStr += " ,  pass: [Y]/N\n";
        }else{
          dataStr += " ,  pass: Y/[N]\n";
        }
      }
    }
    dataStr += "\n";

    this->interface->sendBLEstring( dataStr,  sendTo ); 

    return true;
  }
  // ................................................................
  
  if (this->selected_menu=="$wifi ssid"){  
      this->wifi_ssid = $BLE_CMD;
      this->selected_menu = "$wifi pwd";
      dataStr=  this->wifi_ssid + "\nNetwork password? " + String( char(10));
      this->interface->sendBLEstring( dataStr,  sendTo ); 
      return true;
  }

  if($BLE_CMD=="$wifi ssid"){
    this->selected_menu = "$wifi ssid";
    dataStr= "Network name (SSID) ?" + String( char(10));
    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true;
  }

  if($BLE_CMD.indexOf("$wifi ")>-1){
    dataStr= "Invalid WIFI command" + String( char(10));
    this->interface->sendBLEstring( dataStr,  sendTo ); 
    return true;
  }

  return false;
}

// *******************************************************

bool M_WIFI_CLASS::change_device_name(String $BLE_CMD, uint8_t sendTo ){
  String dataStr="";
  String value = $BLE_CMD.substring(8, $BLE_CMD.length());
  this->interface->config.DEVICE_BLE_NAME= value;
  if (value.length()>20){
    dataStr="max 20 chars allowed\n";
  }else if (this->interface->saveSettings()){
    dataStr = "The Device name is now: "+ value + "\n";
  }else{
    dataStr = "Error saving settings \n";
  }
  this->interface->sendBLEstring( dataStr,  sendTo );  
  return true;
}

// *************************************************************
bool M_WIFI_CLASS::firmware(String $BLE_CMD, uint8_t sendTo ){
  String dataStr="";

  if($BLE_CMD.indexOf("$firmware cfg ")>-1){
    String value= $BLE_CMD.substring(17, $BLE_CMD.length());
    if (value.indexOf("auto")>-1){
      // firmwate update auto
      this->config.firmwareUpdate="auto";
    } else if (value.indexOf("off")>-1){
      // no firware updates
      this->config.firmwareUpdate="no";
    } else if (value.indexOf("on")>-1){
      // manual firmware updates
      this->config.firmwareUpdate="manual";
    }else{
      dataStr="UNK $ CMD \r\n";
      this->interface->sendBLEstring( dataStr,  sendTo );
      return true; 
    }
  }

  if (this->interface->saveSettings()){
    dataStr="$ CMD OK \n";
    this->interface->sendBLEstring( dataStr,  sendTo );  
  }else{
    dataStr="Error saving settings \n";
    this->interface->sendBLEstring( dataStr,  sendTo );  
  }
return true;
}