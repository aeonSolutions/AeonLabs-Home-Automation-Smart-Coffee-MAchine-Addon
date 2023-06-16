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
    this->openOrderRequest = false;

    this->CupOrderSize ="normal";
    this->orderRequestUsername = "xxxxxx";

    this->orderRequestChatID = "xxxxxxxxxx";
    this->orderRequestType = "xxxxxxxx";
    this->orderRequestTime = 0;

    this->OWNER_CHAT_ID = "xxxxxxxxxxx";
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

    if ( this->openOrderRequest == true && ( millis() - this->orderRequestTime > 120000) ){ // 2 min
      this->bot->sendMessage(this->orderRequestChatID , "Miguel did not respond to your offer 🕒. Try again later. ", "");
      this->openOrderRequest = false;
      this->orderRequestChatID = "0000000000";
      this->orderRequestType = "none";
    }

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
    
    // Print the received message
    String text = this->bot->messages[i].text;
    this->interface->mserial->printStrln("Request received ("+chat_id+"): " + text);

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

      if (chat_id == this->OWNER_CHAT_ID){
        welcome += "/accept [short/normal/long] to accept an offer made by another person\n";
        welcome += "\n";
      }
      welcome += "/donation to support this open project\n";
      welcome += "/projectPage to view this open project on Github";

      this->bot->sendMessage(chat_id, welcome, "");
    }
    
    if (chat_id == this->OWNER_CHAT_ID && text.indexOf("/accept")>-1 ){
      if (this->openOrderRequest == true ){
        this->openOrderRequest = false;
        if (this->orderRequestType.equals("decaf") )
          this->orderRequestType = "decaffeinated coffee";      

        String value= text.substring(8, text.length());
        if (value=="normal" || value=="long" || value=="short"){
          this->CupOrderSize = value;
        }else{
          this->CupOrderSize = "normal";
        }
        this->checkMaxDailyCups(this->orderRequestChatID, this->orderRequestUsername, this->orderRequestType);
        this->makeCup(this->orderRequestChatID, this->orderRequestType);
      }else{
        this->bot->sendMessage(this->OWNER_CHAT_ID, "No request was made for brewing a cup. ", "");
      }
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
    

    if (this->openOrderRequest==true){
      if (this->orderRequestChatID  == chat_id){
        this->bot->sendMessage(chat_id, "You already have a pending 🕒 request. Tell Miguel to acccept your offer or wait until he replies. Thank you. 🙏🏼", "");
      }else{
        this->bot->sendMessage(chat_id, "There's already a pending request 🕒 made by someone else. Try again later. Thank you. 🙏🏼", "");
      }
      continue;
    }
    // _________________________________________________________________
    String notifyRequestStr =""; 

    if (text == "/cappuccino") {
      this->bot->sendMessage(chat_id, "You just sent a request for a cup ☕️ of cappuccino to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
      
      notifyRequestStr = from_name + " sent an offer for a cup ☕️ of cappuccino. Do you accept?\n";
      notifyRequestStr += "/accept short  to accept the offer and brew a short cup\n";
      notifyRequestStr += "/accept norma' to accept the offer and brew a normal cup\n";
      notifyRequestStr += "/accept long   to accept the offer and brew a long cup\n";
      notifyRequestStr += "\n";
      this->bot->sendMessage(this->OWNER_CHAT_ID, notifyRequestStr );

      this->orderRequestUsername = from_name;
      this->openOrderRequest = true;
      this->orderRequestChatID = chat_id;
      this->orderRequestType = text.substring(1,text.length());
      this->orderRequestTime=millis();
    }
    
    if (text == "/decaf") {
      this->bot->sendMessage(chat_id, "You just sent a request for a cup ☕️ of decaffeinated coffee to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
      
      notifyRequestStr = from_name + " sent an offer for a cup ☕️ of decaffeinated coffee. Do you accept?\n";
      notifyRequestStr += "/accept short  to accept the offer and brew a short cup\n";
      notifyRequestStr += "/accept norma' to accept the offer and brew a normal cup\n";
      notifyRequestStr += "/accept long   to accept the offer and brew a long cup\n";
      notifyRequestStr += "\n";
      this->bot->sendMessage(this->OWNER_CHAT_ID, notifyRequestStr );

      this->orderRequestUsername = from_name;
      this->openOrderRequest = true;
      this->orderRequestChatID = chat_id;
      this->orderRequestType = text.substring(1,text.length());
      this->orderRequestTime=millis();
    }
    
    if (text == "/coffee") {
      this->bot->sendMessage(chat_id, "You just sent a request for a cup ☕️ of coffee to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
      
      notifyRequestStr = from_name + " sent an offer for a cup ☕️ of coffee. Do you accept?\n";
      notifyRequestStr += "/accept short  to accept the offer and brew a short cup\n";
      notifyRequestStr += "/accept norma' to accept the offer and brew a normal cup\n";
      notifyRequestStr += "/accept long   to accept the offer and brew a long cup\n";
      notifyRequestStr += "\n";
      this->bot->sendMessage(this->OWNER_CHAT_ID, notifyRequestStr );

      this->orderRequestUsername = from_name;
      this->openOrderRequest = true;
      this->orderRequestChatID = chat_id;
      this->orderRequestType = text.substring(1,text.length());
      this->orderRequestTime=millis();
    }
    
    if (text == "/tea") {
      this->bot->sendMessage(chat_id, "You just sent a request for a cup ☕️ of tea to Miguel's " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine", "");
      
      notifyRequestStr = from_name + " sent an offer for a cup ☕️ of tea. Do you accept?\n";
      notifyRequestStr += "/accept short  to accept the offer and brew a short cup\n";
      notifyRequestStr += "/accept norma' to accept the offer and brew a normal cup\n";
      notifyRequestStr += "/accept long   to accept the offer and brew a long cup\n";
      notifyRequestStr += "\n";
      this->bot->sendMessage(this->OWNER_CHAT_ID, notifyRequestStr );

      this->orderRequestUsername = from_name;
      this->openOrderRequest = true;
      this->orderRequestChatID = chat_id;
      this->orderRequestType = text.substring(1,text.length());
      this->orderRequestTime=millis();
    }
    
    if (text == "/state") {
        this->bot->sendMessage(chat_id, "I'm an old " + this->coffeeMachine->coffeeMachineBrand + " Coffee Machine. ", "");
    }
  }
}

/// ******************************
bool TELEGRAM_CLASS::makeCup(String chat_id, String what){
  if( false == this->coffeeMachine->startCoffeeMachine() ) {
    this->bot->sendMessage(chat_id, this->coffeeMachine->errMessage, "");  
    return false;      
  }else{
    this->bot->sendMessage(chat_id, "Miguel accepted your offer. \n Making a " + this->CupOrderSize + " cup of " + what + "...one moment 🕒", "");
    if (false == this->coffeeMachine->startCupFill(what) ){
      this->bot->sendMessage(chat_id, this->coffeeMachine->errMessage, "");
      return false;
    }
    this->bot->sendMessage(chat_id," done. Thank you for your thoughtful offer 👍🏼🙏🏼.", "");
    return true;
  }
}

// ****************************************************
bool TELEGRAM_CLASS:: checkMaxDailyCups( String chat_id, String from_name, String what ){
  if ( what.equals("decaffeinated coffee") || what.equals("coffee") || what.equals("cappuccino") ){    
    if( this->coffeeMachine->config.maxCoffeeCupsPerDay == ( this->coffeeMachine->config.numCoffeeCupsToday +1) ){
      this->bot->sendMessage(chat_id, from_name +", i think this will be my last cup of " + what +" today 👌🏼...", "");  
      this->coffeeMachine->updateNumCoffeeCupsToday();
      return true;   
    }else if ( (this->coffeeMachine->config.numCoffeeCupsToday+1) > this->coffeeMachine->config.maxCoffeeCupsPerDay ){
      this->bot->sendMessage(chat_id, from_name + ", I already had too much " + what +" today 🤪 ... ", "");  
      this->coffeeMachine->updateNumCoffeeCupsToday(); 
      return false;   
    }
    
    this->coffeeMachine->updateNumCoffeeCupsToday(); 
    return true;
  }

  if ( what.equals("tea") ){
    if( this->coffeeMachine->config.maxTeaCupsPerDay ==  (this->coffeeMachine->config.numTeaCupsToday +1) ){
      this->bot->sendMessage(chat_id, from_name +", i think this will be my last cup of " + what +" today 👌🏼...", "");  
      this->coffeeMachine->updateNumTeaCupsToday();
      return true;   
    }else if ( (this->coffeeMachine->config.numTeaCupsToday+1) > this->coffeeMachine->config.maxTeaCupsPerDay  ){
      this->bot->sendMessage(chat_id, from_name + ", I already had too much " + what +" today 🤪 ... ", "");  
      this->coffeeMachine->updateNumTeaCupsToday();
      return false;   
    }

    this->coffeeMachine->updateNumTeaCupsToday();
    return true;
  }

return false; 
}

// ************************************************************