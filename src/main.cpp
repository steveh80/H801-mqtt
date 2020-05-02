#include <Arduino.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "worker.h"
#include "otaupdate.h"

const int CHIP_ID = ESP.getChipId();

#define maxPacketSize (32*2)  //MAX Message Size

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


#define tunableWhiteAdjustment 20

uint8_t increaseValue(uint16_t value, uint16_t adjustment = tunableWhiteAdjustment) {
  if (value >= adjustment) {
    return (value - adjustment) * 100 / (100 - adjustment);
  } else {
    return 0;
  }
}

uint8_t decreaseValue(uint16_t value, uint16_t adjustment = tunableWhiteAdjustment) {
  return min(100, value * 100 / (100 - adjustment));
}

#define onOffthreshold 10

uint8_t scaleSpeed(uint8_t speed, uint8_t dimDelta, uint8_t maxDimDelta) {
  if ((dimDelta == 0) || (speed == 255)) {
    return speed; // already at max speed or no delta at all?
  } else {    
    uint16_t ret = speed % 100; // remove speed multipier, ret should be between 1 and 99 from here on
    ret = ret * maxDimDelta / dimDelta;
    if (speed < 200) { 
      ret = min(ret, (uint16_t) 99);
    } else {
      ret = min(ret, (uint16_t) 54);
    }
    return ret + (speed - (speed % 100));
  } 
}


uint32_t extractNumber(const char *data, int *start, bool *eof) {
  uint32_t result = 0;
  *eof = true;
  while ((*start < maxPacketSize) && (data[*start] >= '0') && (data[*start] <= '9')) {
    *eof = false;
    result = result * 10;
    result = result + uint8_t(data[*start] - '0');
    (*start)++;
  } 
  (*start)++;
  return result;
}

void networkData(char *data, int datalen){
   int p1 = 0;
   uint_dmxChannel startChannel = 0;
   uint32_t newValue = 0;
   uint8_t updSp = 0;
   uint8_t onoffSpeed = 0;
   uint8_t gamma = 0;
   uint8_t d1, d2, d3, dmax = 0;
   bool isOnOff = false;

   if ((datalen > 3) && 
       (data[0] == 'D')  &&
       (data[0 + 1] == 'M')  &&
       (data[0 + 2] == 'X')  &&
       ((data[0 + 3] == 'C') || 
      (data[0 + 3] == 'K') || 
      (data[0 + 3] == 'Z') || 
      (data[0 + 3] == 'Y') || 
      (data[0 + 3] == 'R') || 
      (data[0 + 3] == 'P') || 
      (data[0 + 3] == 'V') || 
      (data[0 + 3] == 'W') ||
      (data[0 + 3] == 'S'))) {
      //  DEBUG_BEGIN(LOG_INFO);
      //  DEBUG_PRINT(F("UDP DATA OK2 Size: "));
      //  DEBUG_PRINT(datalen);
      //  DEBUG_END();
 
     p1 = p1 + 4;
     bool eof = false;
     startChannel = extractNumber(data, &p1, &eof);
     newValue = extractNumber(data, &p1, &eof);
     updSp = extractNumber(data, &p1, &eof);
     if (updSp == 0) {
       updSp = 2;  
     }
     gamma = extractNumber(data, &p1, &eof);
     if (eof) {
       gamma = 2;
     };
     
     onoffSpeed = extractNumber(data, &p1, &eof);
     if (onoffSpeed == 0) {
      onoffSpeed = updSp;
     }
     
     if  (data[0 + 3] == 'R') {
       d1 = queue.add(startChannel, updSp, newValue % 1000, gamma, true); //RED
       newValue = newValue / 1000;
       d2 = queue.add(startChannel + 1, updSp, newValue % 1000, gamma, true); //GREEN
       newValue = newValue / 1000;
       d3 = queue.add(startChannel + 2, updSp, newValue % 1000, gamma, true); //BLUE
       isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold) || (d3 > onOffthreshold);
       if (!isOnOff) {
         onoffSpeed  = updSp;
         isOnOff = true; 
       }
       dmax = max(d1, d2);
       dmax = max(d3, dmax);
      //  DEBUG_BEGIN(LOG_INFO);
      //  DEBUG_PRINT(F("RGB Speed: "));
      //  DEBUG_PRINT(scaleSpeed(onoffSpeed, d1, dmax));
      //  DEBUG_PRINT(F(" "));
      //  DEBUG_PRINT(scaleSpeed(onoffSpeed, d2, dmax));
      //  DEBUG_PRINT(F(" "));
      //  DEBUG_PRINT(scaleSpeed(onoffSpeed, d3, dmax));
      //  DEBUG_END();

       queue.update(startChannel, scaleSpeed(onoffSpeed, d1, dmax), isOnOff);
       queue.update(startChannel + 1, scaleSpeed(onoffSpeed, d2, dmax), isOnOff);
       queue.update(startChannel + 2, scaleSpeed(onoffSpeed, d3, dmax), isOnOff);
     } else if  (data[0 + 3] == 'P') {
       isOnOff = queue.add(startChannel, updSp, newValue, gamma, true) > onOffthreshold;
       queue.update(startChannel, onoffSpeed, isOnOff);
     } else if  (data[0 + 3] == 'W') {
       d1 = queue.add(startChannel, updSp, increaseValue(newValue), gamma, true);                                  /*CW*/
       d2 = queue.add(startChannel + 1, updSp, decreaseValue(newValue), gamma /*or gamma - 1*/, true);             /*WW*/
       isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold);
       queue.update(startChannel, onoffSpeed, isOnOff);
       queue.update(startChannel + 1, onoffSpeed, isOnOff);
     } else if  (data[0 + 3] == 'V') {
       d1 = queue.add(startChannel, updSp, decreaseValue(newValue), gamma /*or gamma - 1*/, true);                 /*WW*/
       d2 = queue.add(startChannel + 1, updSp, increaseValue(newValue), gamma, true);                              /*CW*/
       isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold);
       queue.update(startChannel, onoffSpeed, isOnOff);
       queue.update(startChannel + 1, onoffSpeed, isOnOff);
     } else if  (data[0 + 3] == 'Z') {
       // this mode enabels direct mode for tunable white stripes. like the rgb mode just with 2 channels
       d1 = queue.add(startChannel, updSp, newValue % 1000, gamma, true); //WW
       newValue = newValue / 1000;
       d2 = queue.add(startChannel + 1, updSp, newValue % 1000, gamma, true); //CW
       isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold);
       queue.update(startChannel, onoffSpeed, isOnOff);
       queue.update(startChannel + 1, onoffSpeed, isOnOff);

     } else if  (data[0 + 3] == 'Y') {
       // this mode enabels direct mode for tunable white stripes. like the rgb mode just with 2 channels. (eg. 30060 for 30% / 60% for WW / CW)
       d1 = queue.add(startChannel + 1, updSp, newValue % 1000, gamma, true); //WW
       newValue = newValue / 1000;
       d2 = queue.add(startChannel, updSp, newValue % 1000, gamma, true); //CW
       isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold);
       queue.update(startChannel, onoffSpeed, isOnOff);
       queue.update(startChannel + 1, onoffSpeed, isOnOff);
     
     } else if  (data[0 + 3] == 'K') {
       // this mode uses tunable white information and takes a percentage and a Kelvin value (eg. 302700 => 30% at 2700K)
       // this mode brings the best results for a specfic temperature over all dim levels. works best with H801s high dim resolution of 12bit
       uint16_t colorTemp = newValue % 10000;
       uint16_t percent = newValue / 10000;

       // consider gamma value in percentage to make sure the temperature is the same at each dim level and according to gamma
       percent = _prozToDim(percent, gamma);

       // calculate percentage for each channel
       uint16_t percentWW = (percent*(6500 - colorTemp))/3800;
       uint16_t percentCW = (percent*(colorTemp - 2700))/3800;

       d1 = queue.add(startChannel, updSp, percentWW, 0, false); //WW
       d2 = queue.add(startChannel + 1, updSp, percentCW, 0, false); //CW
       dmax = max(d1, d2);

       isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold);
       queue.update(startChannel, scaleSpeed(onoffSpeed, d1, dmax), isOnOff);
       queue.update(startChannel + 1, scaleSpeed(onoffSpeed, d1, dmax), isOnOff);       
     } else if  (data[0 + 3] == 'S') {
       for (int ch = 0; ch < QUEUESIZE; ch++) {
         queue.add(startChannel + ch, updSp, newValue, gamma, true); 
       };
//     stress = true;
     } else {
       queue.add(startChannel, updSp, newValue, 0, false);
       /*queue.update(startChannel, onoffSpeed, isOnOff);*/
     }
  }
     
  for(int i=0; i<maxPacketSize; i++){
     data[i] = '\0';
  }
  
}




void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.println("got new mqtt message");
  networkData((char *) payload, length);
  Serial.println((char *) payload);
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

  queue.init();
  virt_dmx.init(16);
  worker.init();

  
  mqttClient.setServer(mqtt_server, atoi(mqtt_port));
  mqttClient.setCallback(mqttCallback);
}

boolean mqttReconnect() {
  char announce_topic[] = "H801/announce"; 

  // create a disconnect message as last will
  char will_message[60];
  strcpy(will_message, device_name);
  strcat(will_message, " disconnected");

  if (mqttClient.connect(device_name, mqtt_user, mqtt_pass, announce_topic, 0, 1, will_message)) {
    // Once connected, publish an announcement...
    char message[60];
    strcpy(message, device_name);
    strcat(message, " connected");
    mqttClient.publish(announce_topic, message);

    // ... and subscribe
    char topic[60];
    strcat(topic, "H801/");
    strcat(topic, device_name);
    strcat(topic, "/commands");

    mqttClient.subscribe(topic);
  }
  return mqttClient.connected();
}

void loop() {
  otaUpdate.loop();
  virt_dmx.loop();
  worker.loop();

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