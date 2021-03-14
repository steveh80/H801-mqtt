#include <Arduino.h>
// #include <ESP8266mDNS.h>

#include "settings.h"
#include "otaupdate.h"
#include "wifi.h"
#include "mqtt.h"



void setup() {
    Serial.begin(115200);
    Serial.set_tx(2);

    settings.init();
    wifi.init();

    // if (!MDNS.begin(settings.device_name)) {
    //     Serial.println(F("Error setting up MDNS responder"));
    // }
    // MDNS.addService("http", "tcp", 80);

    mqtt.init();
    otaUpdate.init();
    dimmer.init();
    virt_dmx.init();
}


void loop() {
    wifi.loop();
    // MDNS.update();
    otaUpdate.loop();
    virt_dmx.loop();
    dimmer.loop();
}