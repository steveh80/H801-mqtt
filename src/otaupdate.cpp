#include "otaupdate.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266WebServer httpServer(81);
ESP8266HTTPUpdateServer httpUpdater;


void AddRootPage() {
  httpServer.on("/", HTTP_GET, [&](){
      httpServer.client().setNoDelay(true);
      const char compile_date[] = __DATE__ " " __TIME__;
      String tmp;
      tmp = String("<p>H801 mqtt version ") + String(compile_date) + 
        "<br /><br /><a href=\"/update\">OTA-Update</a><br /></p>";
      httpServer.send(200, F("text/html"), tmp);
      delay(100);
      httpServer.client().stop();
  });    
}


void otaUpdateClass::loop() { 
  httpServer.handleClient();
}

void otaUpdateClass::init() { 
  httpUpdater.setup(&httpServer);
  AddRootPage();
  httpServer.begin();  
}

otaUpdateClass otaUpdate;
