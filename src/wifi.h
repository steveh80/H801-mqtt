#ifndef WIFI_H
#define WIFI_H

#include <DNSServer.h>
#include <WiFiManager.h>
#include "settings.h"

class Wifi
{  
    private:
        WiFiManager wifiManager;
        Settings* settings;

    public:
        void initWithSettings(Settings* settings);
        void loop();
        void resetSettings();
};

extern Wifi wifi;

#endif