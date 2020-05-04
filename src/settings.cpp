#include "settings.h"

void Settings::init() {
    strcat(this->device_name, String(this->chip_id).c_str()); // add chip-ID to default device name

    if (SPIFFS.begin()) {
        if (SPIFFS.exists("/config.json")) {
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);

                StaticJsonDocument<256> doc; // 256 hopefully is enough for everybody, my config has 149 bytes
                DeserializationError err = deserializeJson(doc, buf.get());

                if (err) {
                    Serial.println("failed to load json config");
                } else {
                    strcpy(this->mqtt_server, doc["mqtt_server"]);
                    strcpy(this->mqtt_port, doc["mqtt_port"]);
                    strcpy(this->mqtt_user, doc["mqtt_user"]);
                    strcpy(this->mqtt_pass, doc["mqtt_pass"]);
                    strcpy(this->device_name, doc["device_name"]);
                }
            }
        }
    } else {
        Serial.println("failed to mount file system");
    }
}

void Settings::save() {
    StaticJsonDocument<256> doc; // 256 hopefully is enough for everybody, my config has 149 bytes
    doc["mqtt_server"] = this->mqtt_server;
    doc["mqtt_port"]   = this->mqtt_port;
    doc["mqtt_user"]   = this->mqtt_user;
    doc["mqtt_pass"]   = this->mqtt_pass;
    doc["device_name"] = this->device_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(doc, Serial);
    serializeJson(doc, configFile);
    configFile.close();
}

Settings settings;
