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

#include "onboard_led.h"
#include "Arduino.h"

ONBOARD_LED_CLASS::ONBOARD_LED_CLASS(){
  // ________________ Onborad LED  _____________
  this->LED_RED = 10;
  this->LED_BLUE = 11;
  this->LED_GREEN = 12;

  this->LED_RED_CH = 0;
  this->LED_BLUE_CH = 1;
  this->LED_GREEN_CH = 2;
  
  this->led[0]=  0;
  this->led[0]=  0;
  this->led[0]=  0;
};

void ONBOARD_LED_CLASS::init(){
  ledcSetup(this->LED_GREEN_CH, 4000, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(this->LED_RED_CH, 4000, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(this->LED_BLUE_CH, 4000, 8); // 12 kHz PWM, 8-bit resolution
  
}

void ONBOARD_LED_CLASS::turnOffAllStatusLED(){
    // TURN OFF ALL LED
  ledcDetachPin(this->LED_RED);
  pinMode(this->LED_RED,INPUT);

  ledcDetachPin(this->LED_BLUE);
  pinMode(this->LED_BLUE,INPUT);

  ledcDetachPin(this->LED_GREEN);
  pinMode(this->LED_GREEN,INPUT);

};


void ONBOARD_LED_CLASS::statusLED(  uint8_t brightness, float time) {
  this->turnOffAllStatusLED();

  int8_t ch;
  bool hasRed=false;

  for(int i=0; i < 3; i++){
      if( this->led[i] == this->LED_RED )
        hasRed=true;
  } 

  // turn ON LED
  int bright = floor(255-(brightness/100*255));

  for(int i=0; i < 3; i++){
    if( this->led[i] == this->LED_RED ){
      ch=this->LED_RED_CH;
      pinMode(this->LED_RED,OUTPUT);
      ledcAttachPin(this->LED_RED, this->LED_RED_CH);
      ledcWrite(ch, bright);
    }

    if( this->led[i] == this->LED_GREEN ){
      ch=this->LED_GREEN_CH;
      pinMode(this->LED_GREEN,OUTPUT);
      ledcAttachPin(this->LED_GREEN, this->LED_GREEN_CH);
      if (hasRed)
        bright = floor(255-((brightness*0.7)/100*255));
      ledcWrite(ch, bright);
    }

    if( this->led[i] == this->LED_BLUE ){
      ch=this->LED_BLUE_CH;
      pinMode(this->LED_BLUE,OUTPUT);
      ledcAttachPin(this->LED_BLUE, this->LED_BLUE_CH);
      if (hasRed)
        bright = floor(255-((brightness*0.6)/100*255));
        
      ledcWrite(ch, bright);
    }
  }

  this->led[0]=  0;
  this->led[1]=  0;
  this->led[2]=  0;
  
  if (time>0){
    delay(time*1000);
    this->turnOffAllStatusLED();
  }
}; 

void ONBOARD_LED_CLASS::blinkStatusLED( uint8_t brightness, float time, uint8_t numberBlinks){
  for(int i=0; i<numberBlinks; i++){
    this->statusLED(brightness, time);
    this->turnOffAllStatusLED();
    delay(time*1000);
  }
};