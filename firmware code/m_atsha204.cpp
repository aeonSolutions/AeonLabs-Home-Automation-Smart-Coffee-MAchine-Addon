
#include "m_atsha204.h"
#include "interface_class.h"
#include "sha204_i2c.h"

// *************************************************************

String CryptoGetRandom(INTERFACE_CLASS* interface){
  uint8_t response[32];
  uint8_t returnValue;
  interface->sha204.simpleWakeup();
  returnValue = interface->sha204.simpleGetRandom(response);
  interface->sha204.simpleSleep();  
  return hexDump(response, sizeof(response));
}

String CryptoICserialNumber(INTERFACE_CLASS* interface){
  uint8_t serialNumber[6];
  uint8_t returnValue;

  interface->sha204.simpleWakeup();
  returnValue = interface->sha204.simpleGetSerialNumber(serialNumber);
  interface->sha204.simpleSleep();
  
  return hexDump(serialNumber, sizeof(serialNumber));
}


String macChallengeDataAuthenticity(INTERFACE_CLASS* interface, String text ){
  int str_len = text.length() + 1;
  char text_char [str_len];
  text.toCharArray(text_char, str_len);
        
  static uint32_t n = 0;
  uint8_t mac[32];
  uint8_t challenge[sizeof(text_char)] = {0};

  sprintf((char *)challenge, text_char, n++);

  interface->sha204.simpleWakeup();
  uint8_t ret_code = interface->sha204.simpleMac(challenge, mac);
  interface->sha204.simpleSleep();  
  
  if (ret_code != SHA204_SUCCESS){
    interface->mserial->printStrln("simpleMac failed.");
    return "Error";
  }

  return hexDump(mac, sizeof(mac));
}

String macChallengeDataAuthenticityOffLine(INTERFACE_CLASS* interface, char dataRow[] ){
  static uint32_t n = 0;
  uint8_t mac[32];
  uint8_t challenge[sizeof(dataRow)] = {0}; // MAC_CHALLENGE_SIZE
  
  sprintf((char *)challenge, dataRow, n++);
  interface->mserial->printStr("challenge: ");
  interface->mserial->printStrln((char *)challenge);
  
  uint8_t key[32];
  //Change your key here.
  hex2bin(interface->DATA_VALIDATION_KEY, key);
  uint8_t mac_offline[32];
  interface->sha204.simpleWakeup();
  int ret_code = interface->sha204.simpleMacOffline(challenge, mac_offline, key);
  interface->sha204.simpleSleep();
  interface->mserial->printStr("MAC Offline:\n");
  return hexDump(mac_offline, sizeof(mac_offline));
}



//*********************************************************************
