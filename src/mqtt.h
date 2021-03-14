#include "settings.h"
#include <ESP8266WiFi.h>
#include <PangolinMQTT.h>
#include <Ticker.h>
#include "dimmer.h"

#define RECONNECT_DELAY_M   5
// #define RECONNECT_DELAY_W   5
#define START_WITH_CLEAN_SESSION   true

const char topic_prefix[] = "H801";

class Mqtt {  
    private:
        // long lastReconnectAttempt = 0;

    public:
        void init();  
};

extern Mqtt mqtt;
