#include "otaupdate.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


void OTAUpdate::addIndex() {
  httpServer.on("/", HTTP_GET, [&](){
      httpServer.client().setNoDelay(true);
      const char compile_date[] = __DATE__ " " __TIME__;
      String tmp;
      tmp = String("<p>H801 mqtt version ") + String(compile_date) + 
        "<br>Device name: " + this->device_name + "<br><br><a href=\"/update\">OTA-Update</a><br /></p>";
      httpServer.send(200, F("text/html"), tmp);
      delay(100);
      httpServer.client().stop();
  });
}


void OTAUpdate::loop() { 
  httpServer.handleClient();
}

void OTAUpdate::init() { 
  httpUpdater.setup(&httpServer);
  addIndex();
  httpServer.begin();  
}

void OTAUpdate::setDeviceName(char* device_name) {
  this->device_name = device_name;
}

OTAUpdate otaUpdate;
