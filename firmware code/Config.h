#ifndef CONFIG_H
#define CONFIG_H

#define AP_SSID "SinpleHOme"
#define AP_PASSWORD "123456789"
#define DEFAULT_ROOM_NAME "Room"

#define LOGGING

#define LOOP_DELAY 200
#define PING_INTERVAL 60000
#define AUTO_UPDATE_CYCLES 60

#define SAVED_OR_DEFAULT_ROOM_NAME(string) (strlen(string) == 0 ? DEFAULT_ROOM_NAME : string)

#endif
