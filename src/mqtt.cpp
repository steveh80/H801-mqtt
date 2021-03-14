#include "mqtt.h"

PangolinMQTT mqttClient;
Ticker mqttReconnectTimer, wifiReconnectTimer;

void onMqttConnect(bool sessionPresent) {
    // Once connected, publish an announcement...
    char connect_topic[60];
    snprintf(connect_topic, 60, "%s/%s/%s", topic_prefix, settings.device_name, "connected");
    mqttClient.xPublish(connect_topic, "1", 0, true);

    // publish version
    char version_topic[60];
    const char compile_date[] = __DATE__ " " __TIME__;
    snprintf(version_topic, 60, "%s/%s/%s", topic_prefix, settings.device_name, "version");
    mqttClient.xPublish(version_topic, compile_date, 0, true);

    // ... and subscribe
    char topic[60];
    snprintf(topic, 60, "%s/%s/%s", topic_prefix, settings.device_name, "channel/#");
    Serial.print(F("mqtt subscribe to topic "));
    Serial.println(topic);
    mqttClient.subscribe(topic, 1);
}

void onMqttMessage(const char* topic, const uint8_t* payload, size_t len, uint8_t qos, bool retain, bool dup) {
    // handle message arrived
    Serial.print(F("Got new mqtt message on topic: "));
    Serial.println(topic);

    char* json_str;
    mqttClient.xPayload(payload, len, json_str);
    Serial.printf("payload: %s l=%d\n", json_str, strlen(json_str));

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
    StaticJsonDocument<256> message;
    DeserializationError err = deserializeJson(message, json_str);

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

    free(json_str); // DO NOT FORGET TO DO THIS!!!
}

void connectToMqtt() {
    mqttClient.connect();
}

void connectToWifi() {
    Serial.println("mqtt client is trying reconnect wifi");
    if (!WiFi.isConnected()) {
        WiFi.begin();
    }
}

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
    Serial.print(F("Connected to Wi-Fi as "));
    Serial.println(WiFi.localIP());
    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
    Serial.printf("Disconnected from Wi-Fi event=%d\n",event.reason);
    mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    wifiReconnectTimer.once(RECONNECT_DELAY_W, connectToWifi);
}

void onMqttDisconnect(int8_t reason) {
    Serial.printf("Disconnected from MQTT reason=%d\n",reason);
    mqttReconnectTimer.once(RECONNECT_DELAY_M, connectToMqtt);
}

void onMqttError(uint8_t e,uint32_t info){
  switch(e){
    case TCP_DISCONNECTED:
        // usually because your structure is wrong and you called a function before onMqttConnect
        Serial.printf("ERROR: NOT CONNECTED info=%d\n",info);
        break;
    case MQTT_SERVER_UNAVAILABLE:
        // server has gone away - network problem? server crash?
        Serial.printf("ERROR: MQTT_SERVER_UNAVAILABLE info=%d\n",info);
        break;
    case UNRECOVERABLE_CONNECT_FAIL:
        // there is something wrong with your connection parameters? IP:port incorrect? user credentials typo'd?
        Serial.printf("ERROR: UNRECOVERABLE_CONNECT_FAIL info=%d\n",info);
        break;
    case TLS_BAD_FINGERPRINT:
        Serial.printf("ERROR: TLS_BAD_FINGERPRINT info=%d\n",info);
        break;
    case SUBSCRIBE_FAIL:
        // you tried to subscribe to an invalid topic
        Serial.printf("ERROR: SUBSCRIBE_FAIL info=%d\n",info);
        break;
    case INBOUND_QOS_ACK_FAIL:
        Serial.printf("ERROR: OUTBOUND_QOS_ACK_FAIL id=%d\n",info);
        break;
    case OUTBOUND_QOS_ACK_FAIL:
        Serial.printf("ERROR: OUTBOUND_QOS_ACK_FAIL id=%d\n",info);
        break;
    case INBOUND_PUB_TOO_BIG:
        // someone sent you a p[acket that this MCU does not have enough FLASH to handle
        Serial.printf("ERROR: INBOUND_PUB_TOO_BIG size=%d Max=%d\n",info,mqttClient.getMaxPayloadSize());
        break;
    case OUTBOUND_PUB_TOO_BIG:
        // you tried to send a packet that this MCU does not have enough FLASH to handle
        Serial.printf("ERROR: OUTBOUND_PUB_TOO_BIG size=%d Max=%d\n",info,mqttClient.getMaxPayloadSize());
        break;
    case BOGUS_PACKET: //  Your server sent a control packet type unknown to MQTT 3.1.1 
    //  99.99% unlikely to ever happen, but this message is better than a crash, non? 
        Serial.printf("ERROR: BOGUS_PACKET info=%02x\n",info);
        break;
    case X_INVALID_LENGTH: //  An x function rcvd a msg with an unexpected length: probale data corruption or malicious msg 
    //  99.99% unlikely to ever happen, but this message is better than a crash, non? 
        Serial.printf("ERROR: X_INVALID_LENGTH info=%02x\n",info);
        break;
    case NO_SERVER_DETAILS: //  
    //  99.99% unlikely to ever happen, make sure you call setServer before trying to connect!!!
        Serial.printf("ERROR:NO_SERVER_DETAILS info=%02x\n",info);
        break;
    default:
        Serial.printf("UNKNOWN ERROR: %u extra info %d",e,info);
        break;
  }
}// end error-handling

void Mqtt::init() {
    char connect_topic[60];
    snprintf(connect_topic, 60, "%s/%s/%s", topic_prefix, settings.device_name, "connected");

    mqttClient.setClientId(settings.device_name);
    mqttClient.setServer(settings.mqtt_server, atoi(settings.mqtt_port));
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setWill(connect_topic, 0, true, "0");
    mqttClient.onError(onMqttError);
    mqttClient.setCleanSession(START_WITH_CLEAN_SESSION);
    mqttClient.setKeepAlive(RECONNECT_DELAY_M *3);
    if (settings.mqtt_user != "") mqttClient.setCredentials(settings.mqtt_user, settings.mqtt_pass);

    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    mqttClient.connect();
}

Mqtt mqtt;