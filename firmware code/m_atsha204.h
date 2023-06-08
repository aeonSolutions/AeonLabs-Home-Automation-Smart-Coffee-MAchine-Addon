#include <Arduino.h>
#include "m_math.h"
#include "sha204_i2c.h"
#include "interface_class.h"

#ifndef ATSHA204_DEF
  #define ATSHA204_DEF


String CryptoGetRandom(INTERFACE_CLASS* interface);
String CryptoICserialNumber(INTERFACE_CLASS* interface);

String macChallengeDataAuthenticity(INTERFACE_CLASS* interface, String text );
String macChallengeDataAuthenticityOffLine(INTERFACE_CLASS* interface, char dataRow[] );


#endif