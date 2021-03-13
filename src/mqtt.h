#define MQTT_KEEPALIVE = 360

#include "settings.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "dimmer.h"

class Mqtt {  
    private:
        long lastReconnectAttempt = 0;

    public:
        void init();  
        void loop();
        boolean reconnect();
};

extern Mqtt mqtt;
