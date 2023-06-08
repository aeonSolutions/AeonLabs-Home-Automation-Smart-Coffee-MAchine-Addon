#ifndef ROUTES_H
#define ROUTES_H

#include <WebServer.h>
#include "m_file_class.h"
#include "m_wifi.h"
#include "interface_class.h"

#define HTML_HEAD "<head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Settings</title><link rel='stylesheet' href='/css'></head>"
#define MIME_HTML F("text/html")

class Routes {
  public:
    FILE_CLASS* drive; 
    M_WIFI_CLASS* mWifi;
    INTERFACE_CLASS* interface;

    Routes(WebServer* webServer);
    void init(INTERFACE_CLASS* interface, FILE_CLASS* drive, M_WIFI_CLASS* mWifi );

    void handleRoot();
    void handleWiFi();
    void handleWiFiScript();
    void handleWiFiResult();
    void handleWiFiSave();
    void handleRoomName();
    void handleRoomNameSave();
    void handleWeather();
    void handleWeatherSave();
    void handleRequestRestart();
    void handleStatus();
    void handleCommand();
    void handleCss();
    void handleNotFound();
    static bool shouldRestart;
  private:
    WebServer* server;
};

#endif
