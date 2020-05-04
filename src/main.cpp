#include <Arduino.h>
#include <ESP8266mDNS.h>

#include "settings.h"
#include "otaupdate.h"
#include "wifi.h"
#include "mqtt.h"


void setup() {
    Serial.begin(115200);
    Serial.set_tx(2);

    settings.init();
    
    wifi.initWithSettings(&settings);

    if (!MDNS.begin(settings.device_name)) {
        Serial.println("Error setting up MDNS responder");
    }
    MDNS.addService("http", "tcp", 80);

    otaUpdate.init();
    otaUpdate.setDeviceName(settings.device_name);

    mqtt.initWithSettings(&settings);

    dimmer.init();
    virt_dmx.init();
}


void loop() {
    MDNS.update();
    otaUpdate.loop();
    virt_dmx.loop();
    dimmer.loop();
    mqtt.loop();
}