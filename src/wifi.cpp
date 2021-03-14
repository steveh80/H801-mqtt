#include "wifi.h"

bool shouldSaveConfig = false;
void saveConfigCallback () {
    shouldSaveConfig = true;
}

void Wifi::init() { 
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", settings.mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", settings.mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_user("user", "mqtt username", "", 20);
    WiFiManagerParameter custom_mqtt_pass("pass", "mqtt password", "", 20);
    WiFiManagerParameter custom_device_name("devicename", "device name", settings.device_name, 30);

    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
    wifiManager.addParameter(&custom_device_name);

    this->connect();

    Serial.println(F("Wifi connected"));

    if ( shouldSaveConfig ) {
        strcpy(settings.mqtt_server, custom_mqtt_server.getValue());
        strcpy(settings.mqtt_port, custom_mqtt_port.getValue());
        strcpy(settings.mqtt_user, custom_mqtt_user.getValue());
        strcpy(settings.mqtt_pass, custom_mqtt_pass.getValue());
        strcpy(settings.device_name, custom_device_name.getValue());
        settings.save();
    }
}

void Wifi::connect() {
    wifiManager.setHostname(settings.device_name);
    wifiManager.setConfigPortalTimeout(120);

    // This will block until a WiFi is connected, or the timeout has elapsed
    if ( !wifiManager.autoConnect("H801") ) {
        Serial.println(F("failed to connect and hit timeout"));
        delay(3000);
        ESP.restart();
        delay(5000);
    }
}

void Wifi::resetSettings() {
    Serial.println(F("Wifimanager settings are about to be deleted!"));
    settings.remove();
    wifiManager.resetSettings();
}

void Wifi::loop() { 
}

Wifi wifi;