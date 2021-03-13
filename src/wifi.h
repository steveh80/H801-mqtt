#pragma once

#include <DNSServer.h>
#include <WiFiManager.h>
#include "settings.h"

class Wifi
{  
    private:
        WiFiManager wifiManager;

    public:
        void init();
        void connect();
        void loop();
        void resetSettings();
};

extern Wifi wifi;