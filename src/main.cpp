#include <Arduino.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>
#include <PubSubClient.h>

#include "worker.h"
#include "otaupdate.h"

#define maxPacketSize (32*2)  //MAX Message Size

const char* mqtt_server = "192.168.1.8";
const char* mqtt_username = "H801";
const char* mqtt_password = "LcXbM4j";

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient mqttClient(mqtt_server, 1883, espClient);





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
  networkData((char *) payload, length);
}


void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    // Create a random client ID
    String clientId = "H801-testing";
    
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      // Once connected, publish an announcement...
      mqttClient.publish("H801/announce", "H801 connected");
      // ... and resubscribe
      mqttClient.subscribe("H801/commands");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("starting wifi manager");  
  wifiManager.autoConnect("H801");
  // wifiManager.setTimeout(180);
  // if (!wifiManager.autoConnect("H801")) {
  //   delay(3000);
  //   ESP.reset();
  //   delay(5000);
  // }
  Serial.println("connected");

  mqttClient.setCallback(mqttCallback);

  otaUpdate.init();
  queue.init();
  virt_dmx.init(16);
  worker.init();
}

void loop() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  otaUpdate.loop();
  virt_dmx.loop();
  worker.loop();
}
