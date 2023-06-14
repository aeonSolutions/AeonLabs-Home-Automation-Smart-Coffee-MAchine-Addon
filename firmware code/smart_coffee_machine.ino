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

#define uS_TO_S_FACTOR 1000000

//----------------------------------------------------------------------------------------
// Components Testing  **************************************
bool SCAN_I2C_BUS = true;
bool TEST_FINGERPRINT_ID_IC = false;

//----------------------------------------------------------------------------------
#include <math.h>
#include <cmath>
#include "SPI.h"
#include <semphr.h>

#include "esp32-hal-psram.h"
// #include "rom/cache.h"
extern "C"
{
#include <esp_himem.h>
#include <esp_spiram.h>
}

#include <driver/i2c.h>

// custom includes **********************************
#include "nvs_flash.h"  //preferences lib

// External sensor moeasurements
#include "telegram.h"
TELEGRAM_CLASS* telegram = new TELEGRAM_CLASS();

// custom functions
#include "m_file_functions.h"

// Interface class ******************************
#include "interface_class.h"
INTERFACE_CLASS* interface = new INTERFACE_CLASS();
#define DEVICE_NAME "Smart Coffee Machine"

// GBRL commands  ***************************
#include "gbrl.h"
GBRL gbrl = GBRL();

// Onboard sensors  *******************************
#include "onboard_sensors.h"
ONBOARD_SENSORS* onBoardSensors = new ONBOARD_SENSORS();

// unique figerprint data ID
#include "m_atsha204.h"

// serial comm
#include <HardwareSerial.h>
HardwareSerial UARTserial(0);

#include "mserial.h"
mSerial* mserial = new mSerial(true, &UARTserial);

// File class
#include <esp_partition.h>
#include "FS.h"
#include <LittleFS.h>
#include "m_file_class.h"

FILE_CLASS* drive = new FILE_CLASS(mserial);

// WIFI Class
#include <ESP32Ping.h>
#include "m_wifi.h"
M_WIFI_CLASS* mWifi = new M_WIFI_CLASS();


// Certificates
#include "github_cert.h"

// Coffee Machine 
#include "coffee_machine.h"
COFFEE_MACHINE_CLASS* coffeeMachine = new COFFEE_MACHINE_CLASS();

/********************************************************************/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pCharacteristicTX, *pCharacteristicRX;
BLEServer *pServer;
BLEService *pService;


bool BLE_advertise_Started = false;

bool newValueToSend = false;
String $BLE_CMD = "";
bool newBLESerialCommandArrived = false;
SemaphoreHandle_t MemLockSemaphoreBLE_RX = xSemaphoreCreateMutex();

float txValue = 0;
String valueReceived = "";


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      mWifi->setBLEconnectivityStatus (true);
      mserial->printStr("BLE connection init ", mserial->DEBUG_BOTH_USB_UART);

      interface->onBoardLED->led[0] = interface->onBoardLED->LED_BLUE;
      interface->onBoardLED->statusLED(100, 1);

      String dataStr = "Connected to the Smart Concrete Maturity device (" + String(interface->firmware_version) + ")" + String(char(10)) + String(char(13)) + "Type $? or $help to see a list of available commands" + String(char(10));
      dataStr += String(interface->rtc.getDateTime(true)) + String(char(10)) + String(char(10));

      if (mWifi->getNumberWIFIconfigured() == 0 ) {
        dataStr += "no WiFi Networks Configured" + String(char(10)) + String(char(10));
      }
      //interface->sendBLEstring(dataStr, mserial->DEBUG_TO_BLE);
    }

    void onDisconnect(BLEServer* pServer) {
      mWifi->setBLEconnectivityStatus (false);

      interface->onBoardLED->led[0] = interface->onBoardLED->LED_BLUE;
      interface->onBoardLED->statusLED(100, 0.5);
      interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
      interface->onBoardLED->statusLED(100, 0.5);
      interface->onBoardLED->led[0] = interface->onBoardLED->LED_BLUE;
      interface->onBoardLED->statusLED(100, 0.5);

      pServer->getAdvertising()->start();
    }
};

class pCharacteristicTX_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String txValue = String(pCharacteristic->getValue().c_str());
      txValue.trim();
      mserial->printStrln("Transmitted TX Value: " + String(txValue.c_str()) , mserial->DEBUG_BOTH_USB_UART);

      if (txValue.length() == 0) {
        mserial->printStr("Transmitted TX Value: empty ", mserial->DEBUG_BOTH_USB_UART);
      }
    }

    void onRead(BLECharacteristic *pCharacteristic) {
      mserial->printStr("TX onRead...", mserial->DEBUG_BOTH_USB_UART);
      //pCharacteristic->setValue("OK");
    }
};

class pCharacteristicRX_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      delay(10);

      String rxValue = String(pCharacteristic->getValue().c_str());
      rxValue.trim();
      mserial->printStrln("Received RX Value: " + String(rxValue.c_str()), mserial->DEBUG_BOTH_USB_UART );

      if (rxValue.length() == 0) {
        mserial->printStr("Received RX Value: empty " , mserial->DEBUG_BOTH_USB_UART);
      }

      $BLE_CMD = rxValue;
      mWifi->setBLEconnectivityStatus(true);

      xSemaphoreTake(MemLockSemaphoreBLE_RX, portMAX_DELAY);
      newBLESerialCommandArrived = true; // this needs to be the last line
      xSemaphoreGive(MemLockSemaphoreBLE_RX);

      delay(50);
    }

    void onRead(BLECharacteristic *pCharacteristic) {
      mserial->printStr("RX onRead..." , mserial->DEBUG_BOTH_USB_UART);
      //pCharacteristic->setValue("OK");
    }

};

// ********************************************************
// *************************  == SETUP == *****************
// ********************************************************
//variaveis que indicam o núcleo
static uint8_t taskCoreZero = 0;
static uint8_t taskCoreOne  = 1;

long int prevMeasurementMillis;

void setup() {
  ESP_ERROR_CHECK(nvs_flash_erase());
  nvs_flash_init();

  // Firmware Build Version / revision ______________________________
  interface->firmware_version = "1.0.0";

  // Serial Communication Init ______________________________
  interface->UARTserial = &UARTserial;
  mserial->DEBUG_TO = mserial->DEBUG_TO_UART;
  mserial->DEBUG_EN = true;
  mserial->DEBUG_TYPE = mserial->DEBUG_TYPE_VERBOSE; // DEBUG_TYPE_INFO;

  mserial->start(115200);

  // ......................................................................................................
  // .......................... START OF IO & PIN CONFIGURATION..............................................
  // ......................................................................................................

  // I2C IOs  __________________________
  interface->I2C_SDA_IO_PIN = 9;
  interface->I2C_SCL_IO_PIN = 8;

  // Power Saving ____________________________________
  interface->LIGHT_SLEEP_EN = false;


  // ________________ Onboard LED  _____________
  interface->onBoardLED = new ONBOARD_LED_CLASS();
  interface->onBoardLED->LED_RED = 36;
  interface->onBoardLED->LED_BLUE = 34;
  interface->onBoardLED->LED_GREEN = 35;

  interface->onBoardLED->LED_RED_CH = 8;
  interface->onBoardLED->LED_BLUE_CH = 6;
  interface->onBoardLED->LED_GREEN_CH = 7;

  // ___________ MCU freq ____________________
  interface-> SAMPLING_FREQUENCY = 240;
  interface-> WIFI_FREQUENCY = 80; // min WIFI MCU Freq is 80-240
  interface->MIN_MCU_FREQUENCY = 10;
  interface-> SERIAL_DEFAULT_SPEED = 115200;

  // _____________________ coffee machine ________________________
  /* for IO assignment edit the coffee machine class constructor */
  
  coffeeMachine->coffeeMachineBrand = "Philips Senseo";

// _____________________ TELEGRAM _____________________________
  telegram->CHAT_ID = "1435561519";
  // Initialize Telegram BOT
  telegram->BOTtoken = "5813926838:AAFwC1cV_QghdZiVUP8lAwbg9mNvkWc27jA";  // your Bot Token (Get from Botfather)

  // ......................................................................................................
  // .......................... END OF IO & PIN CONFIGURATION..............................................
  // ......................................................................................................

  interface->onBoardLED->init();

  interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
  interface->onBoardLED->statusLED(100, 0);

  //init storage drive ___________________________
  drive->partition_info();
  if (drive->init(LittleFS, "storage", 2, mserial,   interface->onBoardLED ) == false)
    while (1);

  //init interface ___________________________
  interface->init(mserial, true); // debug EN ON
  interface->settings_defaults();

  if ( !interface->loadSettings() ) {
    interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
    interface->onBoardLED->led[0] = interface->onBoardLED->LED_GREEN;
    interface->onBoardLED->statusLED(100, 2);
  }


  // init onboard sensors ___________________________
  onBoardSensors->init(interface, mserial);

  if (SCAN_I2C_BUS) {
    // onBoardSensors->I2Cscanner();

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = interface->I2C_SDA_IO_PIN;
    conf.scl_io_num = interface->I2C_SCL_IO_PIN;
    conf.sda_pullup_en = false;
    conf.scl_pullup_en = false;
    conf.master.clk_speed = 400000;
    i2c_param_config(I2C_NUM_0, &conf);

    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

    esp_err_t res;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    printf("00:         ");
    for (uint8_t i = 3; i < 0x78; i++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
        i2c_master_stop(cmd);

        res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
        if (i % 16 == 0)
            printf("\n%.2x:", i);
        if (res == 0)
            printf(" %.2x", i);
        else
            printf(" --");
        i2c_cmd_link_delete(cmd);
    }
    printf("\n\n");

  }


  if (TEST_FINGERPRINT_ID_IC) {
    mserial->printStrln("Testing the Unique FingerPrind ID for Sensor Data Measurements");

    mserial->printStr("This Smart Device  Serial Number is : ");
    mserial->printStrln(CryptoICserialNumber(interface));

    mserial->printStrln("Testing Random Genenator: " + CryptoGetRandom(interface));
    mserial->printStrln("");

    mserial->printStrln("Testing Sensor Data Validation hashing");
    mserial->printStrln( macChallengeDataAuthenticity(interface, "TEST IC"));
    mserial->printStrln("");
  }

  mserial->printStrln("\nMicrocontroller specifications:");
  interface->CURRENT_CLOCK_FREQUENCY = getCpuFrequencyMhz();
  mserial->printStr("Internal Clock Freq = ");
  mserial->printStr(String(interface->CURRENT_CLOCK_FREQUENCY));
  mserial->printStrln(" MHz");

  interface->Freq = getXtalFrequencyMhz();
  mserial->printStr("XTAL Freq = ");
  mserial->printStr(String(interface->Freq));
  mserial->printStrln(" MHz");

  interface->Freq = getApbFrequency();
  mserial->printStr("APB Freq = ");
  mserial->printStr(String(interface->Freq / 1000000));
  mserial->printStrln(" MHz\n");

  interface->setMCUclockFrequency( interface->WIFI_FREQUENCY);
  mserial->printStrln("setting Boot MCU Freq to " + String(getCpuFrequencyMhz()) +"MHz");
  mserial->printStrln("");

  // init BLE
  BLE_init();

  //init wifi
  mWifi->init(interface, drive, interface->onBoardLED);
  mWifi->OTA_FIRMWARE_SERVER_URL = "https://github.com/aeonSolutions/AeonLabs-Home-Automation-Smart-Coffee-MAchine-Addon/releases/download/openFirmware/firmware.bin";
  
  mWifi->add_wifi_network("TheScientist", "angelaalmeidasantossilva");
  
  mWifi->ALWAYS_ON_WIFI=true;
  mWifi->WIFIscanNetworks();
  
  mWifi->start(10000,5);

  // check for firmwate update
  mWifi->startFirmwareUpdate();

  interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
  interface->onBoardLED->statusLED(100, 0);
  
  // initialize the coffee machine
  coffeeMachine->init(interface);
  
  // initialize Telegram
  telegram->init(interface, mWifi, coffeeMachine);
  
  //Init GBRL
  gbrl.init(interface, mWifi);

  interface->onBoardLED->led[0] = interface->onBoardLED->LED_BLUE;
  interface->onBoardLED->statusLED(100, 0);

  interface->$espunixtimePrev = millis();
  interface->$espunixtimeStartMeasure = millis();

  mWifi->$espunixtimeDeviceDisconnected = millis();

  prevMeasurementMillis = millis();
  
  mserial->printStr("Starting MCU cores... ");
/*
  xTaskCreatePinnedToCore (
    loop2,     // Function to implement the task
    "loop2",   // Name of the task
    1000,      // Stack size in bytes
    NULL,      // Task input parameter
    0,         // Priority of the task
    NULL,      // Task handle.
    0          // Core where the task should run
  );
*/
  MemLockSemaphoreBLE_RX = xSemaphoreCreateMutex();
  mserial->printStrln("done. ");

  mserial->printStrln("Free memory: " + addThousandSeparators( std::string( String(esp_get_free_heap_size() ).c_str() ) ) + " bytes");
  mserial->printStrln("\nSetup is completed. You may start using the " + String(DEVICE_NAME) );
  mserial->printStrln("Type $? for a List of commands.\n");

  interface->onBoardLED->led[0] = interface->onBoardLED->LED_GREEN;
  interface->onBoardLED->statusLED(100, 1);

}
// ********END SETUP *********************************************************

void GBRLcommands(String command, uint8_t sendTo) {
  if (gbrl.commands(command, sendTo) == false) {
    if ( onBoardSensors->gbrl_commands(command, sendTo ) == false) {
      if (mWifi->gbrl_commands(command, sendTo ) == false) {
            if ( command.indexOf("$") > -1) {
              interface->sendBLEstring("$ CMD ERROR \r\n", sendTo);
            } else {
              // interface->sendBLEstring("$ CMD UNK \r\n", sendTo);
            }
          }
        }
      }
    }

// ************************************************************

void BLE_init() {
  // Create the BLE Device
  BLEDevice::init(String("LDAD " + interface->config.DEVICE_BLE_NAME).c_str());  // max 29 chars

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic

  pCharacteristicTX = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );


  pCharacteristicTX->addDescriptor(new BLE2902());

  pCharacteristicRX = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                      );

  pCharacteristicTX->setCallbacks(new pCharacteristicTX_Callbacks());
  pCharacteristicRX->setCallbacks(new pCharacteristicRX_Callbacks());

  interface->init_BLE(pCharacteristicTX);

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
}
// *******************************************************************************************

//******************************* ==  LOOP == ******************************************************

unsigned long lastMillisWIFI = 0;
int waitTimeWIFI = 0;

String dataStr = "";
long int eTime;
long int statusTime = millis();
long int beacon = millis();
// adc_power_release()

unsigned int cycle = 0;
uint8_t updateCycle = 0;


// ********************* == Core 1 : Data Measurements Acquisition == ******************************
void loop2 (void* pvParameters) {
  //measurements->runExternalMeasurements();
  delay(2000);
}


//************************** == Core 2: Connectivity WIFI & BLE == ***********************************************************
void loop(){  
  if (millis() - beacon > 1000) {    
    beacon = millis();
    mserial->printStrln("(" + String(beacon) + ") Free memory: " + addThousandSeparators( std::string( String(esp_get_free_heap_size() ).c_str() ) ) + " bytes\n", mSerial::DEBUG_TYPE_VERBOSE, mSerial::DEBUG_ALL_USB_UART_BLE);
  }

  if ( (millis() - statusTime > 10000)) { //10 sec
    statusTime = millis();
    interface->onBoardLED->led[1] = interface->onBoardLED->LED_GREEN;
    interface->onBoardLED->statusLED(100, 0.04);
  } else if  (millis() - statusTime > 10000) {
    statusTime = millis();
    interface->onBoardLED->led[1] = interface->onBoardLED->LED_RED;
    interface->onBoardLED->statusLED(100, 0.04);
  }
  
  // .............................................................................
  // disconnected for at least 3min
  // change MCU freq to min
  if (  mWifi->getBLEconnectivityStatus() == false && ( millis() - mWifi->$espunixtimeDeviceDisconnected > 180000) && interface->CURRENT_CLOCK_FREQUENCY >= interface->WIFI_FREQUENCY) {
    mserial->printStrln("setting min MCU freq.");
    btStop();
    //BLEDevice::deinit(); // crashes the device

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_MODE_NULL);

    interface->setMCUclockFrequency(interface->MIN_MCU_FREQUENCY);
    mWifi->setBLEconnectivityStatus(false);

    interface->onBoardLED->led[0] = interface->onBoardLED->LED_RED;
    interface->onBoardLED->led[1] = interface->onBoardLED->LED_GREEN;
    interface->onBoardLED->statusLED(100, 2);
  }

  // ................................................................................
  // Ligth Sleeep
  eTime = millis() - prevMeasurementMillis;
  if ( mWifi->getBLEconnectivityStatus() == false && interface->LIGHT_SLEEP_EN) {
    mserial->printStr("Entering light sleep....");
    interface->onBoardLED->turnOffAllStatusLED();

    //esp_sleep_enable_timer_wakeup( ( (measurements->config.MEASUREMENT_INTERVAL - eTime) / 1000)  * uS_TO_S_FACTOR);
    delay(100);
    esp_light_sleep_start();
    mserial->printStrln("wake up done.");
  }
  prevMeasurementMillis = millis();

  // .....................................................................
  // Telegram
  telegram->runTelegramBot();

  // ................................................................................    

  if (mserial->readSerialData()){
    GBRLcommands(mserial->serialDataReceived, mserial->DEBUG_TO_USB);
  }
  // ................................................................................    

  if (mserial->readUARTserialData()){
    GBRLcommands(mserial->serialUartDataReceived, mserial->DEBUG_TO_UART);
  }
  // ................................................................................

  if (newBLESerialCommandArrived){
    xSemaphoreTake(MemLockSemaphoreBLE_RX, portMAX_DELAY); 
      newBLESerialCommandArrived=false; // this needs to be the last line       
    xSemaphoreGive(MemLockSemaphoreBLE_RX);

    GBRLcommands($BLE_CMD, mserial->DEBUG_TO_BLE);
  }

  // ....................................................................................
  // OTA Firmware
  if ( mWifi->forceFirmwareUpdate == true )
    mWifi->startFirmwareUpdate();

// ---------------------------------------------------------------------------
  if (millis() - lastMillisWIFI > 60000) {
    xSemaphoreTake(interface->MemLockSemaphoreCore2, portMAX_DELAY);
    waitTimeWIFI++;
    lastMillisWIFI = millis();
    xSemaphoreGive(interface->MemLockSemaphoreCore2);
  }

}
// -----------------------------------------------------------------
