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


TELEGRAM_CLASS::TELEGRAM_CLASS() {
    this->lastTimeBotRan = 0;

    this->CHAT_ID = "xxxxxxxxxxx";
    // Initialize Telegram BOT
    this->BOTtoken = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";  // your Bot Token (Get from Botfather)
}

void TELEGRAM_CLASS::init(INTERFACE_CLASS* interface, M_WIFI_CLASS* mWifi, COFFEE_MACHINE_CLASS* coffeeMachine ){
  this->interface=interface;
  this->interface->mserial->printStr("\ninit Telegram ...");
  this->mWifi= mWifi;
  this->coffeeMachine = coffeeMachine;
  
  this->botRequestDelay = 1000;

  this->client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  this->bot = new UniversalTelegramBot(this->BOTtoken, this->client);

  this->interface->mserial->printStrln("done.");
}
// ************************************************************
void TELEGRAM_CLASS::runTelegramBot(){

    if (millis() > this->lastTimeBotRan + this->botRequestDelay)  {
        mWifi->start(10000,5);

        int numNewMessages = this->bot->getUpdates(this->bot->last_message_received + 1);

        while(numNewMessages) {
            this->handleNewMessages(numNewMessages);
            numNewMessages = this->bot->getUpdates(this->bot->last_message_received + 1);
        }
        this->lastTimeBotRan = millis();
    }
}
// ***********************************************************************************************
// Handle what happens when you receive new messages
void TELEGRAM_CLASS::handleNewMessages(int numNewMessages) {
  //Serial.println("handleNewMessages");
  //Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(this->bot->messages[i].chat_id);
    if (chat_id != this->CHAT_ID){
      this->bot->sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = this->bot->messages[i].text;
    this->interface->mserial->printStrln("Request received: " + text);

    String from_name = this->bot->messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to order Miguel a cup of coffee.\n\n";
      welcome += "/coffee to buy a cup of Coffee \n";
      welcome += "/tea to buy a cup of Tea\n";
      welcome += "/cappuccino to buy a cup of Cappuccino\n";
      welcome += "/decaf to buy a cup of decaf. coffee\n";
      welcome += "\n";
      welcome += "/state to request current coffee machine state \n";
      welcome += "\n";
      welcome += "/donation to support this open project\n";
      welcome += "/projectPage to view this open project on Github";
      this->bot->sendMessage(chat_id, welcome, "");
    }

    if (text == "/projectPage") {
      String msg ="This smart coffee machine open project is available on my github here:\n";
      msg += "https://github.com/aeonSolutions/AeonLabs-Home-Automation-Smart-Coffee-MAchine-Addon/tree/main\n";
      this->bot->sendMessage(chat_id, msg, "");
    }
    
    if (text == "/donation") {
      String msg ="To support this open hardware & open source project you can make a donation using:\n";
      msg += "- Paypal: mtpsilva@gmail.com\n";
      msg += "- or at https://www.buymeacoffee.com/migueltomas";
      this->bot->sendMessage(chat_id, msg, "");
    }
    
    if (text == "/cappuccino") {
      this->bot->sendMessage(chat_id, "You just sent a cup of cappuccino request to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
    }
    
    if (text == "/decaf") {
      this->bot->sendMessage(chat_id, "You just sent a cup of decaf. coffee request to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
    }
    
    if (text == "/coffee") {
      this->bot->sendMessage(chat_id, "You just sent a cup of coffee request to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
      
      if( false == this->coffeeMachine->startCoffeeMachine() ) {
        this->bot->sendMessage(chat_id, this->coffeeMachine->errMessage, "");        
      }else{
        this->bot->sendMessage(chat_id, "Miguel accepted your offer. \n Making a cup of Coffee...one moment", "");
        if (false == this->coffeeMachine->MakeNewCoffee() ){
          this->bot->sendMessage(chat_id, this->coffeeMachine->errMessage, "");
        }
      }
    }
    
    if (text == "/tea") {
      this->bot->sendMessage(chat_id, "You just sent a cup of tea request to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
    }
    
    if (text == "/state") {
        this->bot->sendMessage(chat_id, "I'm an old " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine. ", "");
    }
  }
}
