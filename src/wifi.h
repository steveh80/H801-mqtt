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
};

extern Wifi wifi;
