#include "wifi.h"

bool shouldSaveConfig = false;
void saveConfigCallback () {
    shouldSaveConfig = true;
}

void Wifi::initWithSettings(Settings* settings) { 
    this->settings = settings;

    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", settings->mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", settings->mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_user("user", "mqtt username", "", 20);
    WiFiManagerParameter custom_mqtt_pass("pass", "mqtt password", "", 20);
    WiFiManagerParameter custom_device_name("devicename", "device name", settings->device_name, 30);

    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
    wifiManager.addParameter(&custom_device_name);

    wifiManager.setTimeout(120);
    wifiManager.autoConnect("H801");

    if (!wifiManager.autoConnect("H801")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        ESP.reset();
        delay(5000);
    }
    Serial.println("connected");

    strcpy(settings->mqtt_server, custom_mqtt_server.getValue());
    strcpy(settings->mqtt_port, custom_mqtt_port.getValue());
    strcpy(settings->mqtt_user, custom_mqtt_user.getValue());
    strcpy(settings->mqtt_pass, custom_mqtt_pass.getValue());
    strcpy(settings->device_name, custom_device_name.getValue());

    if ( shouldSaveConfig ) {
        settings->save();
    }
}

void Wifi::resetSettings() {
    Serial.println("Wifimanager settings are about to be deleted!");
    this->settings->remove();
    wifiManager.resetSettings();
}

void Wifi::loop() { 
}


Wifi wifi;