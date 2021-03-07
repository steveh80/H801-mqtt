#include "mqtt.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    // handle message arrived
    Serial.println("got new mqtt message");
    Serial.println(topic);
    Serial.println((char *)payload);

    // retrieve channel information from topic
    char *channel_str = strrchr(topic, '-');
    int channel = 0;
    if (channel_str) {
        channel_str = channel_str + 1;
        channel = atoi(channel_str);
    } else {
        Serial.println("could not retrieve proper channel");
        return;
    }
    Serial.println("Channel: ");
    Serial.println(channel);

    // parse json object
    StaticJsonDocument<256> message; // 256 hopefully is enough for everybody, my config has 149 bytes
    DeserializationError err = deserializeJson(message, (char *)payload, length);

    if (err) {
        Serial.println("got broken json message");
        Serial.println(err.c_str());
        return;
    }

    //default values for params
    uint8_t speed = (message["speed"] == NULL ? 2 : message["speed"]);

    if (message["mode"] == "single") {
        dimmer.dimChannel(channel, message["bri"], speed, message["curve"], message["onOffSpeed"]);
    }

    if (message["mode"] == "cct") {
        dimmer.dimCCT(channel, message["bri"], message["colortemp"], speed, message["curve"], message["onOffSpeed"]);
    }

    if (message["mode"] == "lumitech-cct") {
        int lumitech = message["lumitech"];
        lumitech = constrain(lumitech, 200002700, 201006500); // allow onlytunable white values
        lumitech = lumitech - 200000000;                      // remove lumitech "header"

        uint8_t brightness = lumitech / 10000;
        uint16_t colortemp = lumitech % 10000;
        dimmer.dimCCT(channel, brightness, colortemp, speed, message["curve"], message["onOffSpeed"]);
    }

    if (message["mode"] == "rgb") {
        dimmer.dimRGB(channel, message["rgb"], speed, message["curve"], message["onOffSpeed"]);
    }
}

void Mqtt::initWithSettings(Settings *settings) {
    this->settings = settings;
    mqttClient.setServer(settings->mqtt_server, atoi(settings->mqtt_port));
    mqttClient.setCallback(mqttCallback);
}

void Mqtt::loop() {
    if (!mqttClient.connected()) {
        if (!WiFi.isConnected()) {
            return;
        }
        long now = millis();
        if (now - this->lastReconnectAttempt > 5000) {
            this->lastReconnectAttempt = now;
            // Attempt to reconnect
            if (this->reconnect()) {
                this->lastReconnectAttempt = 0;
            }
        }
    } else {
        // Client connected
        mqttClient.loop();
    }
}

boolean Mqtt::reconnect() {
    char announce_topic[] = "H801/announce";

    // create a disconnect message as last will
    char will_message[60];
    strcpy(will_message, settings->device_name);
    strcat(will_message, " disconnected");

    if (mqttClient.connect(settings->device_name, settings->mqtt_user, settings->mqtt_pass, announce_topic, 0, 1, will_message, false)) {
        // Once connected, publish an announcement...
        char message[60];
        strcpy(message, settings->device_name);
        strcat(message, " connected");
        mqttClient.publish(announce_topic, message);

        // ... and subscribe
        char topic[60];
        strcat(topic, "H801/");
        strcat(topic, settings->device_name);
        strcat(topic, "/#");

        mqttClient.subscribe(topic, 1);
    }
    return mqttClient.connected();
}

Mqtt mqtt;