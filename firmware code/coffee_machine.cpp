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

#include "telegram.h"
#include "Arduino.h"


COFFEE_MACHINE_CLASS::COFFEE_MACHINE_CLASS() {
    this->errMessage="";

    this->WATER_LEVEL_IO =14;

    this->BOILER_MAX_TEMP_DATA_IO = 7;
    this->BOILER_MAX_TEMP_CS_IO = 10;
    this->BOILER_MAX_TEMP_SK_IO = 11;

    this->BOILER_DS18B20_TEMP_IO = 1;

    this->COFFEE_BUTTON_IO = 6;
    this->BOILER_BUTTON_IO = 4;
    this->GRINDER_BUTTON_IO = 5;
}

// ************************************************************
void COFFEE_MACHINE_CLASS::init(INTERFACE_CLASS* interface){
  this->interface=interface;
  this->interface->mserial->printStr("\ninit Philips Senseo coffee machine ...");

  // ADC 
  pinMode(  this->WATER_LEVEL_IO , INPUT);
  pinMode(  this->BOILER_DS18B20_TEMP_IO , INPUT);

  pinMode(  this->COFFEE_BUTTON_IO , OUTPUT);
  pinMode(  this->BOILER_BUTTON_IO , OUTPUT);
  digitalWrite( this->COFFEE_BUTTON_IO,HIGH);
  digitalWrite( this->BOILER_BUTTON_IO,HIGH); // disabled

  this->boilerTemperature = new DS18B20_SENSOR();
  this->boilerTemperature->init(this->interface,  this->BOILER_DS18B20_TEMP_IO);

  this->boilerTemperature->ProbeSensorStatus();

  this->coffeeCup = new VL6180X_SENSOR();
  this->coffeeCup->init(this->interface);
  this->coffeeCup->startVL6180X();

  this->interface->mserial->printStrln("init done.");
}

// **************************************************
void COFFEE_MACHINE_CLASS::ToggleWaterHeaterButton(int state){
  digitalWrite( this->BOILER_BUTTON_IO,state); // disabled
}

// **************************************************
void COFFEE_MACHINE_CLASS::ToggleCoffeeButton(int state){
  digitalWrite( this->COFFEE_BUTTON_IO,state);
  
}


// ************************************************************
bool COFFEE_MACHINE_CLASS::IsLowWaterLevel(){
  int waterLevelRaw = analogRead(this->WATER_LEVEL_IO);
  
  this->interface->mserial->printStrln("Current (RAW) water level val: " + String(waterLevelRaw));
  
  if ( waterLevelRaw = 4095) {
    return false; // water level ok
  }else{
    return true; // low water level 
  }

}

// ************************************************************
float COFFEE_MACHINE_CLASS::requestWaterTemperature(){
   this->boilerTemperature->requestMeasurements();
  return  this->boilerTemperature->measurement[0];
}

// ************************************************************
bool COFFEE_MACHINE_CLASS::heatBoiler(){

}

// *********************************************
uint8_t COFFEE_MACHINE_CLASS::readCoffeeHeight(){
  this->coffeeCup->requestMeasurements();
  return this->coffeeCup->measurement[1];
}

// ************************************************************
uint8_t COFFEE_MACHINE_CLASS::getCoffeeHeight(){
  return this->userCoffeeHeight;
}

// ************************************************************
void COFFEE_MACHINE_CLASS::setCoffeeHeight(uint8_t userCoffeeHeight){
  this->userCoffeeHeight = userCoffeeHeight;
}

// ************************************************************
bool COFFEE_MACHINE_CLASS::checkCoffeeCupIsPlaced(){
  // ToDo
  return true;
}

// ************************************************************
bool COFFEE_MACHINE_CLASS::startCoffeeMachine(){
  this->errMessage = "";

  if (this->IsLowWaterLevel()){
    this->errMessage = "The coffee machine needs a water refill.";
    this->interface->mserial->printStrln( this->errMessage ); 
    return false;
  
  }else{
    this->interface->mserial->printStrln( "Water level is OK."); 
  }

  this->ToggleWaterHeaterButton(LOW);
  
  long int timeout = millis();

  while( this->requestWaterTemperature() < 40.0 && ( millis()-timeout < 120000 ) ){ // 2min
      this->interface->mserial->printStrln( "Boiler Temp: " + String(this->requestWaterTemperature()) + "*C" );
      delay(2000);
  }
  this->ToggleWaterHeaterButton(HIGH);
  if (this->requestWaterTemperature() < 40.0 && ( millis()-timeout >= 120000 ) ){
    this->errMessage = "The Coffee Machine boiler is not heating.";
    this->interface->mserial->printStrln( this->errMessage ); 
    //return false;
  }

  // check if there is a coffee cup / mug
  if ( this->checkCoffeeCupIsPlaced() == false){
    this->errMessage = "There's no cup/mug on the machine.";
    this->interface->mserial->printStrln( this->errMessage ); 
    //return false;
  }else{
        this->interface->mserial->printStrln("found coffee /mug."); 
  }
  return true;
}

// *************************************************************
bool COFFEE_MACHINE_CLASS::MakeNewCoffee(){
  this->interface->mserial->printStrln( "Making a cup of coffee...."); 

// turn on the water pump 
  this->ToggleCoffeeButton(LOW);
  
  long int timeout = millis();
  while( this->readCoffeeHeight() < 40.0 && ( millis()-timeout < 120000 ) ){ // 2min
      this->interface->mserial->printStrln( "coffee height is: " + String(this->readCoffeeHeight()) + "mm" );
      delay(2000);
  }

  this->ToggleWaterHeaterButton(HIGH);
  
  if (this->readCoffeeHeight() < 40.0 && ( millis()-timeout >= 120000 ) ){
    this->errMessage = "The Coffee Machine cup sensor is not working.";
    this->interface->mserial->printStrln( this->errMessage ); 
    //return false;
  }

  this->ToggleCoffeeButton(HIGH);  

  return true;
}