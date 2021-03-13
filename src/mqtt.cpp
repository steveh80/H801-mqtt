#include "mqtt.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    // handle message arrived
    Serial.print(F("Got new mqtt message on topic: "));
    Serial.println(topic);
    Serial.print(F("Payload: "));
    Serial.println((char *)payload);

    // retrieve channel information from topic
    char *channel_str = strrchr(topic, '/');
    int channel = 0;
    if (channel_str) {
        channel_str = channel_str + 1;
        channel = atoi(channel_str);
    } else {
        Serial.println(F("could not retrieve proper channel"));
        return;
    }
    Serial.print(F("Channel: "));
    Serial.println(channel);

    // parse json object
    StaticJsonDocument<128> message;
    DeserializationError err = deserializeJson(message, (char *)payload, length);

    if (err) {
        Serial.println(F("got broken json message"));
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

void Mqtt::init() {
    mqttClient.setServer(settings.mqtt_server, atoi(settings.mqtt_port));
    mqttClient.setCallback(mqttCallback);
}

void Mqtt::loop() {
    if (!mqttClient.connected()) {
        Serial.println(F("MQTT disconnected"));
        Serial.print(F("free heap: "));
        Serial.println(ESP.getFreeHeap(),DEC);

        Serial.print(F("heap fragmentation: "));
        Serial.println(ESP.getHeapFragmentation(),DEC);


        // this should improve resilience in bad wifi situations, but it causes to break reconnects to mqtt broker
        // if ( !WiFi.isConnected()) {
        //     Serial.print(F("Wifi connection status in mqtt::loop is: "));
        //     Serial.println(WiFi.status());
        //     return;
        // }
        long now = millis();
        if (now - this->lastReconnectAttempt > 5000) {
            this->lastReconnectAttempt = now;
            // Attempt to reconnect
            Serial.println(F("Attempting mqtt connect"));
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
    const char compile_date[] = __DATE__ " " __TIME__;

    // prepare base topic
    char base_topic[30];
    strcat(base_topic, "H801/");
    strcat(base_topic, settings.device_name);
    strcat(base_topic, "/"); 

    char connect_topic[60];
    strcpy(connect_topic, base_topic);
    strcat(connect_topic, "connected");

    // create a disconnect message as last will
    const char will_message[] = "0";

    if (mqttClient.connect(settings.device_name, settings.mqtt_user, settings.mqtt_pass, connect_topic, 0, true, will_message, false)) { 
        // Once connected, publish an announcement...
        mqttClient.publish(connect_topic, "1", true);

        // publish version
        char version_topic[60];
        strcpy(version_topic, base_topic);
        strcat(version_topic, "version");
        mqttClient.publish(version_topic, compile_date, true);

        // ... and subscribe
        char topic[60];
        strcpy(topic, base_topic);
        strcat(topic, "channel/#"); 
        Serial.print(F("mqtt subscribe to topic "));
        Serial.println(topic);
        mqttClient.subscribe(topic, 1);
    }
    return mqttClient.connected();
}

Mqtt mqtt;