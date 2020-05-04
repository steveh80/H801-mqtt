#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "dimmer.h"
#include "settings.h"

class Mqtt {  
    private:
        Settings* settings;
        long lastConnectedTimestamp = 0;

    public:
        void initWithSettings(Settings* settings);  
        void loop();
        boolean reconnect();
};

extern Mqtt mqtt;
