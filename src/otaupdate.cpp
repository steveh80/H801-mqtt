#include "otaupdate.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


void OTAUpdate::addRoutes() {
    httpServer.on("/", HTTP_GET, [&]() {
        httpServer.client().setNoDelay(true);
        const char compile_date[] = __DATE__ " " __TIME__;
        String tmp = String("<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'></head>") +
            "<body><div class='container'><h1>H801 mqtt</h1>" +
            "Version: " + String(compile_date) +
            "<br>Device name: " + settings->device_name +
            "<br>MQTT server: " + settings->mqtt_server + ":" + settings->mqtt_port +
            "<br>MQTT User: " + settings->mqtt_user +
            "</p><p><a href='update' class='btn btn-primary'>OTA-Update</a> " +
            "<a href='reboot' class='btn btn-warning' onclick=\"return confirm('Are you sure to reboot?')\">Device reboot</a> " +
            "<a href='reset' class='btn btn-danger' onclick=\"return confirm('Are you sure to factory-reset?')\">Factory-Reset</a></p>" +
            "</div></body></html>";

        httpServer.send(200, F("text/html"), tmp);
        delay(100);
        httpServer.client().stop();
    });

    httpServer.on("/reset", HTTP_GET, [&]() {
        String tmp = String("<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'>") +
            "<meta http-equiv='refresh' content='10; /'>" +
            "</head><body><div class='container'><h1>H801 mqtt</h1>" +
            "<div class='alert alert-danger'>Reset of H801 was successful, you can reconfigure it. </div>" + 
            "</div></body></html>";
        httpServer.send(200, F("text/html"), tmp);
        delay(1000);
        httpServer.client().stop();
        this->wifi->resetSettings();
        ESP.reset();
    });

    httpServer.on("/reboot", HTTP_GET, [&]() {
        String tmp = String("<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'>") +
            "<meta http-equiv='refresh' content='10; /'>" +
            "</head><body><div class='container'><h1>H801 mqtt</h1>" +
            "<div class='alert alert-danger'>Device is rebooting.</div>" + 
            "</div></body></html>";
        httpServer.send(200, F("text/html"), tmp);
        httpServer.client().stop();
        ESP.reset();
    });
}

void OTAUpdate::loop() {
    httpServer.handleClient();
}

void OTAUpdate::initWithSettings(Settings *settings) {
    this->settings = settings;
    httpUpdater.setup(&httpServer);
    this->addRoutes();
    httpServer.begin();
}

void OTAUpdate::addWifi(Wifi* wifi) {
    this->wifi = wifi;
}

OTAUpdate otaUpdate;
