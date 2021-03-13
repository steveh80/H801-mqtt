#include "settings.h"

void Settings::init() {
    strcat(this->device_name, String(this->chip_id).c_str()); // add chip-ID to default device name

    if (SPIFFS.begin()) {
        if (SPIFFS.exists(F("/config.json"))) {
            File configFile = SPIFFS.open(F("/config.json"), "r");
            if (configFile) {
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);

                StaticJsonDocument<256> doc; // 256 hopefully is enough for everybody, my config has 149 bytes
                DeserializationError err = deserializeJson(doc, buf.get());

                if (err) {
                    Serial.println(F("failed to load json config"));
                } else {
                    Serial.println(F("Settings were loaded:"));
                    serializeJson(doc, Serial);
                    
                    strcpy(this->mqtt_server, doc[F("mqtt_server")]);
                    strcpy(this->mqtt_port, doc[F("mqtt_port")]);
                    strcpy(this->mqtt_user, doc[F("mqtt_user")].as<char*>());
                    strcpy(this->mqtt_pass, doc[F("mqtt_pass")].as<char*>());
                    strcpy(this->device_name, doc[F("device_name")]);
                }
            }
        }
    } else {
        Serial.println(F("failed to mount file system"));
    }
}

void Settings::save() {
    Serial.println(F("Settings are about to be saved."));
    StaticJsonDocument<256> doc; // 256 hopefully is enough for everybody, my config has 149 bytes
    doc[F("mqtt_server")] = this->mqtt_server;
    doc[F("mqtt_port")]   = this->mqtt_port;
    doc[F("mqtt_user")]   = this->mqtt_user;
    doc[F("mqtt_pass")]   = this->mqtt_pass;
    doc[F("device_name")] = this->device_name;

    File configFile = SPIFFS.open(F("/config.json"), "w");
    if (!configFile) {
      Serial.println(F("failed to open config file for writing"));
    }

    serializeJson(doc, Serial);
    serializeJson(doc, configFile);
    configFile.close();
}

void Settings::remove() {
    SPIFFS.remove(F("/config.json"));
}

Settings settings;
