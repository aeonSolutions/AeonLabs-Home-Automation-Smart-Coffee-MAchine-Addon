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

#include "m_file_class.h"
#include <esp_partition.h>
#include "FS.h"
#include <LittleFS.h>
#include "mserial.h"
#include "m_file_functions.h"

FILE_CLASS::FILE_CLASS(mSerial* mserial){
  this->mserial = mserial;
}

bool FILE_CLASS::init(fs::LittleFSFS &fs, String partitionName, uint8_t maxFiles, mSerial* mserial, ONBOARD_LED_CLASS* onboardLED){
    this->mserial=mserial;
    this->onBoardLED=onBoardLED;
    
    //this->mserial->printStrln("init drive 1...");
    //this->mserial->printStrln("Onboard LED RED:" +  String(this->onBoardLED->led[0] ) );
    //this->mserial->printStrln("init drive 2...");
    //this->mserial->printStrln("Onboard LED RED:" + String(this->onBoardLED->led[0]) + " = " + String(this->onBoardLED->LED_RED ) );
    //this->mserial->printStrln("init drive 3...");


    if(true==fs.begin( false, "/storage", maxFiles , "storage")){ // !! Only allow one file to be open at a time instead of 10, saving 9x4 - 36KB of RAM          
      this->mserial->printStrln( "Drive init...done.");
    //this->onBoardLED->led[0] = this->onBoardLED->LED_GREEN;
    //this->onBoardLED->statusLED(100, 1);
    }else if(fs.format()){
        this->mserial->printStrln("Drive formated sucessfully!");
      //  this->onBoardLED->led[0] = this->onBoardLED->LED_RED;
      //  this->onBoardLED->led[0] = this->onBoardLED->LED_GREEN;
      //  this->onBoardLED->statusLED(100, 2);
        if(true==fs.begin( false, "/storage", maxFiles , "storage")){
          this->mserial->printStrln( "Drive init...done.");
       // this->onBoardLED->led[0] = this->onBoardLED->LED_GREEN;
       // this->onBoardLED->statusLED(100, 1);
        }else{
          this->mserial->printStrln("Drive Mount Failed");
          return false;
        }   

    }else{
        this->mserial->printStrln("Drive init Failed");
     //   this->onBoardLED->led[0] = this->onBoardLED->LED_RED;
     //   this->mserial->printStrln("here 1");
     //   this->onBoardLED->statusLED(100, 5);
     //   this->mserial->printStrln("here 2");
        return false;
    }

    this->mserial->printStrln("Total space: " + addThousandSeparators( std::string( String(fs.totalBytes() ).c_str() ) ) + " bytes");
    this->mserial->printStrln("used space: " + addThousandSeparators( std::string( String(fs.usedBytes() ).c_str() ) )  + " bytes\n" );
    return true;
}

// ************************************************************
void FILE_CLASS::partition_info(){
    this->mserial->printStrln("\nStorage Partition list:", mSerial::DEBUG_TYPE_VERBOSE, mSerial::DEBUG_ALL_USB_UART_BLE);
    partloop(ESP_PARTITION_TYPE_DATA);
    partloop(ESP_PARTITION_TYPE_APP);

}

void FILE_CLASS::partloop(esp_partition_type_t part_type) {
  esp_partition_iterator_t iterator = NULL;
  const esp_partition_t *next_partition = NULL;
  iterator = esp_partition_find(part_type, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (iterator) {
     next_partition = esp_partition_get(iterator);
     if (next_partition != NULL) {
      String dataStr = "partition addr: 0x" +String(next_partition->address, HEX) + "    size: " + addThousandSeparators( std::string( String(next_partition->size, DEC).c_str() ) )  + " bytes     label: " + String(next_partition->label) ;  
      this->mserial->printStrln(dataStr, mSerial::DEBUG_TYPE_VERBOSE, mSerial::DEBUG_ALL_USB_UART_BLE);
      iterator = esp_partition_next(iterator);
    }
  }
}

//***********************************************************
void FILE_CLASS::storage_list_files(fs::FS &fs){
    this->mserial->printStrln("Listing Files and Directories: ");
    // Open dir folder
    auto dir = fs.open("/storage");
    // Cycle all the content
    onBoardLED->led[1] = onBoardLED->LED_RED;
    onBoardLED->statusLED(100,0); 
    this->printDirectory(dir,1);
    onBoardLED->led[1] = onBoardLED->LED_GREEN;
    onBoardLED->statusLED(100,2); 
    this->mserial->printStrln("");
}

//***********************************************************
bool FILE_CLASS::storage_test_write_file(fs::FS &fs){    
    auto testFile = fs.open(F("/storage/testCreate.txt"), "w"); 
    if (testFile){
      this->mserial->printStr("Writing content to a file...");
      testFile.print("Here the test text!!");
      testFile.close();
      this->mserial->printStrln("DONE.");
      return true;
    }else{
      this->mserial->printStrln("Error openning file(1) !");
      onBoardLED->led[1] = onBoardLED->LED_RED;
      onBoardLED->statusLED(100,5); 
      return false;
    }
}

//***********************************************************
bool FILE_CLASS::storage_test_read_file(fs::FS &fs){
    auto testFile = fs.open(F("/storage/testCreate.txt"), "r");
    if (testFile){
      this->mserial->printStrln("-- Reading file content (below) --");
      this->mserial->printStrln(testFile.readString());
      testFile.close();
      this->mserial->printStrln("DONE.");
      this->mserial->printStrln("-- Reading File completed --");
      return true;
    }else{
      this->mserial->printStrln("Error reading file !");
      onBoardLED->led[1] = onBoardLED->LED_RED;
      onBoardLED->statusLED(100,5); 
      return false;
    }
}


//***********************************************************
void FILE_CLASS::printDirectory(File dir, int numTabs) {
  while (true) {
    auto entry =  dir.openNextFile();
    if (! entry) {
      this->mserial->printStrln("");
      this->mserial->printStrln("no more files or directories found.");
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      this->mserial->printStr("     ");
    }
    this->mserial->printStr(entry.name());
    if (entry.isDirectory()) {
      this->mserial->printStrln("/");
      this->printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      this->mserial->printStr("          ");
      this->mserial->printStrln(String(entry.size()));
    }
    entry.close();
  }
}
// *********************************************
String FILE_CLASS::readFile(String filename, fs::FS &fs){
  
  this->mserial->printStrln("reading file...");

  File readFile = fs.open(filename, FILE_READ); 
  if ( !readFile ){
    this->mserial->printStrln("error opening  file " + filename);
    readFile.close();
    return "false";
  }
return readFile.readString();
}
// *************************************************************

bool FILE_CLASS::writeFile(String filename, String contents, fs::FS &fs){
  
  this->mserial->printStrln("Saving file...");

  if (fs.exists(filename) )
    fs.remove(filename);

  File saveFile = fs.open(filename, FILE_READ); 
  if ( !saveFile ){
    this->mserial->printStrln("error opening  file " + filename);
    saveFile.close();
    return false;
  }

  saveFile.print(contents);
  return true;
}