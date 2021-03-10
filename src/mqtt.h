#define MQTT_KEEPALIVE = 360

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "dimmer.h"
#include "settings.h"

class Mqtt {  
    private:
        Settings* settings;
        long lastReconnectAttempt = 0;

    public:
        void initWithSettings(Settings* settings);  
        void loop();
        boolean reconnect();
};

extern Mqtt mqtt;
