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
#include "interface_class.h"
#include "max6675.h"
#include "src/sensors/ds18b20.h"
#include "src/sensors/vl6180x.h"

#ifndef COFFEE_MACHINE_DEF  
  #define COFFEE_MACHINE_DEF
  

  class COFFEE_MACHINE_CLASS {
    private:
      INTERFACE_CLASS* interface=nullptr;
      MAX6675* boilerTemp;
      DS18B20_SENSOR* boilerTemperature;
      VL6180X_SENSOR* coffeeCup;
      
    public:
        uint8_t WATER_LEVEL_IO;

        uint8_t COFFEE_BUTTON_IO;
        uint8_t BOILER_BUTTON_IO;
        uint8_t GRINDER_BUTTON_IO;

        uint8_t BOILER_MAX_TEMP_DATA_IO;
        uint8_t BOILER_MAX_TEMP_CS_IO;
        uint8_t BOILER_MAX_TEMP_SK_IO;

        uint8_t BOILER_DS18B20_TEMP_IO;

        String errMessage;
        String coffeeMachineBrand;
        uint8_t userCoffeeHeight;
        uint8_t coffeeCupHeight;

        COFFEE_MACHINE_CLASS();
        void init(INTERFACE_CLASS* interface);

        bool startCoffeeMachine();
        bool startCupFill(String what);

        bool MakeNewCoffee();
        bool MakeNewDecaf();
        bool MakeNewCappuccino();
        bool MakeNewTea();

        bool IsLowWaterLevel();

        float requestWaterTemperature();
        bool heatBoiler();
        
        uint8_t getUserCoffeeHeight();
        void setUserCoffeeHeight(uint8_t userCoffeeHeight);
        
        bool checkCoffeeCupIsPlaced();
        uint8_t readCoffeeHeight();
        void setCoffeeCupHeight();

        void ToggleCoffeeButton(int state);
        void ToggleWaterHeaterButton(int state);
  };

#endif
