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
#include "sqlite/sqlite3.h"

#ifndef INTERFACE_CLASS_DEF
  #include "interface_class.h"
#endif

#ifndef ONBOARD_LED_CLASS_DEF
  #include "onboard_led.h"
#endif

#ifndef MSERIAL_DEF
  #include "mserial.h"
#endif

#ifndef SETTINGS_CLASS_DEF
  #define SETTINGS_CLASS_DEF

class SETTINGS_CLASS {
  private:
    sqlite3 *db_settings;
    sqlite3_stmt *result;
    char *tail;

    int db_exec(sqlite3 *db, const char *sql);
    int db_open(const char *filename, sqlite3 **db);

    const char* data = "Callback function called";
    //static int callback(void *data, int argc, char **argv, char **azColName);

  public:
    mSerial*            mserial=nullptr;
    INTERFACE_CLASS*    interface=nullptr;
    ONBOARD_LED_CLASS*  onBoardLED;


    SETTINGS_CLASS();
    
    void init(INTERFACE_CLASS* interface, mSerial* mserial,  ONBOARD_LED_CLASS* onBoardLED);
    void load();

    void query(String query);

};

#endif