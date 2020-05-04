#include <Arduino.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "otaupdate.h"
#include "dimmer.h"

const int CHIP_ID = ESP.getChipId();

// default values for mqtt form
char mqtt_server[40]      = "mqtt.server.de";
char mqtt_port[6]         = "1883";
char mqtt_user[20]        = "username";
char mqtt_pass[20]        = "password";
char device_name[40]      = "H801-"; // will later pre suffixed with device chip-id
long lastConnectedTimestamp = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool shouldSaveConfig = false;

void saveConfigCallback () {
  shouldSaveConfig = true;
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.println("got new mqtt message");
  Serial.println(topic);
  Serial.println((char *) payload);
  
  // retrieve channel information from topic
  char *channel_str = strrchr(topic, '-');
  int channel = 0;
  if (channel_str) {
      channel_str = channel_str+1;
      channel = atoi(channel_str);
  } else {
    Serial.println("could not retrieve proper channel");
    return;
  }
  Serial.println("Channel: ");
  Serial.println(channel);

  // parse json object
  StaticJsonDocument<256> message; // 256 hopefully is enough for everybody, my config has 149 bytes
  DeserializationError err = deserializeJson(message, (char *) payload, length);
  
  if (err) {
    Serial.println("got broken json message");
    Serial.println(err.c_str());
    return;
  }

  //default values for params
  uint8_t speed = ( message["speed"] == NULL ? 2 : message["speed"]);

  if (message["mode"] == "single") {
    dimmer.dimChannel(channel, message["bri"], speed, message["curve"], message["onOffSpeed"]);
  }

  if (message["mode"] == "cct") {
    dimmer.dimCCT(channel, message["bri"], message["colortemp"], speed, message["curve"], message["onOffSpeed"]);
  }

  if (message["mode"] == "lumitech-cct") {
    int lumitech = message["lumitech"];
    lumitech = constrain(lumitech, 200002700, 201006500); // allow onlytunable white values
    lumitech = lumitech - 200000000; // remove lumitech "header"

    uint8_t brightness = lumitech / 10000;
    uint16_t colortemp = lumitech % 10000;
    dimmer.dimCCT(channel, brightness, colortemp, speed, message["curve"], message["onOffSpeed"]);
  }

  if (message["mode"] == "rgb") {
    dimmer.dimRGB(channel, message["rgb"], speed, message["curve"], message["onOffSpeed"]);
  }
}


void setup() {
  Serial.begin(115200);
  Serial.set_tx(2);
  // while(!Serial) { delay(100); }

  Serial.println();
  Serial.print("chip id: ");
  Serial.println(CHIP_ID);

  strcat(device_name, String(CHIP_ID).c_str()); // add chip-ID to default device name

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
          strcpy(mqtt_server, doc["mqtt_server"]);
          strcpy(mqtt_port, doc["mqtt_port"]);
          strcpy(mqtt_user, doc["mqtt_user"]);
          strcpy(mqtt_pass, doc["mqtt_pass"]);
          strcpy(device_name, doc["device_name"]);
        }
      }
    }
  } else {
    Serial.println("failed to mount file system");
  }

  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt username", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt password", mqtt_pass, 20);
  WiFiManagerParameter custom_device_name("devicename", "device name", device_name, 30);

  WiFiManager wifiManager;
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

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(device_name, custom_device_name.getValue());
  

  if (shouldSaveConfig) {
    Serial.println("saving config");

    StaticJsonDocument<256> doc; // 256 hopefully is enough for everybody, my config has 149 bytes
    doc["mqtt_server"] = mqtt_server;
    doc["mqtt_port"] = mqtt_port;
    doc["mqtt_user"] = mqtt_user;
    doc["mqtt_pass"] = mqtt_pass;
    doc["device_name"] = device_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(doc, Serial);
    serializeJson(doc, configFile);
    configFile.close();
  }


  otaUpdate.init();

  dimmer.init();
  virt_dmx.init(16);

  
  mqttClient.setServer(mqtt_server, atoi(mqtt_port));
  mqttClient.setCallback(mqttCallback);
}

boolean mqttReconnect() {
  char announce_topic[] = "H801/announce"; 

  // create a disconnect message as last will
  char will_message[60];
  strcpy(will_message, device_name);
  strcat(will_message, " disconnected");

  if (mqttClient.connect(device_name, mqtt_user, mqtt_pass, announce_topic, 0, 1, will_message, false)) {
    // Once connected, publish an announcement...
    char message[60];
    strcpy(message, device_name);
    strcat(message, " connected");
    mqttClient.publish(announce_topic, message);

    // ... and subscribe
    char topic[60];
    strcat(topic, "H801/");
    strcat(topic, device_name);
    strcat(topic, "/#");

    mqttClient.subscribe(topic, 1);
  }
  return mqttClient.connected();
}

void loop() {
  otaUpdate.loop();
  virt_dmx.loop();
  dimmer.loop();

  // Problem: Wenn der H801 mit mqtt verbunden ist, schlägt der Update fehl.
  // Was passiert? Die mqtt Verbindung wird getrennt, und möchte beim nächsten loop direkt wieder verbinden. 
  // Findet das update während den 5 sekunden Karenz-Zeit statt, funktioniert es, weil der H801 nicht wieder zum mqtt broker verbinden kann.

  long now = millis();
  if (!mqttClient.connected()) {
    if (now - lastConnectedTimestamp > 5000) {
      // Attempt to reconnect
      mqttReconnect();
      lastConnectedTimestamp = 0;
    }
  } else {
    // Client connected
    lastConnectedTimestamp = now;
    mqttClient.loop();
  }
}