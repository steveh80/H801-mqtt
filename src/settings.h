#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>

class Settings
{  
  private:

  public:
    // default values
    char mqtt_server[40]      = "mqtt.server.de";
    char mqtt_port[6]         = "1883";
    char mqtt_user[20]        = "username";
    char mqtt_pass[20]        = "password";
    char device_name[40]      = "H801-"; // will later pre suffixed with device chip-id

    int chip_id = ESP.getChipId();

    void init();
    void save();
    void remove();
};

extern Settings settings;

#endif